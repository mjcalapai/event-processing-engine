// MLFQ-inspired weighted severity scheduler
#include "mlfq_scheduler.h"

MLFQScheduler::MLFQScheduler(int capacity)
    : capacity(capacity), count(0), serviceCounter(0) {
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&not_full, nullptr);
    pthread_cond_init(&not_empty, nullptr);
}

MLFQScheduler::~MLFQScheduler() {
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);
}

int MLFQScheduler::severityToQueue(Severity s) {
    switch (s) {
        case Severity::CRITICAL: return 0;
        case Severity::ERROR:    return 1;
        case Severity::WARNING:  return 2;
        case Severity::INFO:     return 3;
        case Severity::DEBUG:    return 4;
    }

    return 4;
}

void MLFQScheduler::append(LogEntry* item) {
    pthread_mutex_lock(&lock);

    while (count == capacity) {
        pthread_cond_wait(&not_full, &lock);
    }

    if (item == nullptr) {
        queues[0].push_back(nullptr);
    } else {
        int q = severityToQueue(item->severity);
        queues[q].push_back(item);
    }

    count++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}

int MLFQScheduler::chooseQueue() {
    /*
      Weighted MLFQ-style policy:
      - Critical gets frequent service
      - Error gets frequent service
      - Lower queues still get scheduled
      - Prevents pure-priority starvation
    */

    static const int schedule[] = {
        0, 0, 0, 0,   // CRITICAL
        1, 1, 1,      // ERROR
        2, 2,         // WARNING
        3,            // INFO
        4             // DEBUG
    };

    static const int scheduleSize = sizeof(schedule) / sizeof(schedule[0]);

    for (int attempt = 0; attempt < scheduleSize; attempt++) {
        int idx = (serviceCounter + attempt) % scheduleSize;
        int q = schedule[idx];

        if (!queues[q].empty()) {
            serviceCounter = (idx + 1) % scheduleSize;
            return q;
        }
    }

    return -1;
}

LogEntry* MLFQScheduler::remove() {
    pthread_mutex_lock(&lock);

    while (count == 0) {
        pthread_cond_wait(&not_empty, &lock);
    }

    int q = chooseQueue();

    LogEntry* item = queues[q].front();
    queues[q].pop_front();
    count--;

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&lock);

    return item;
}

bool MLFQScheduler::isEmpty() {
    pthread_mutex_lock(&lock);
    bool empty = (count == 0);
    pthread_mutex_unlock(&lock);
    return empty;
}