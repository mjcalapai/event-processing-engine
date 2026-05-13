// MLFQ-inspired weighted severity scheduler
#include "mlfq_scheduler.h"
#include "metrics.h"



MLFQScheduler::MLFQScheduler(int capacity, MLFQPolicy policy)
    : capacity(capacity), count(0), serviceCounter(0), policy(policy) {
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
        QueuedLog poison;
        poison.entry = nullptr;
        poison.enqueueTime = std::chrono::steady_clock::now();
        queues[0].push_back(poison);
    } else {
        int q = severityToQueue(item->severity);
        // queues[q].push_back(item);
        QueuedLog ql;
        ql.entry = item;
        ql.enqueueTime = std::chrono::steady_clock::now();

        queues[q].push_back(ql);
    }

    count++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}


void MLFQScheduler::applyAgingBoost() {
    auto now = std::chrono::steady_clock::now();
    metrics.recordAgingBoost();

    for (int q = NUM_QUEUES - 1; q > 0; q--) {

        auto& curQueue = queues[q];

        for (auto it = curQueue.begin(); it != curQueue.end();) {

            auto waited =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->enqueueTime
                ).count();

            if (waited >= agingThresholdSeconds[q]) {

                QueuedLog promoted = *it;

                it = curQueue.erase(it);

                queues[q - 1].push_back(promoted);

            } else {
                ++it;
            }
        }
    }
}


int MLFQScheduler::chooseQueue() {
    if (policy == MLFQPolicy::RR) {
        static const int rrSchedule[] = {
            0, 1, 2, 3, 4
        };

        static const int rrSize = sizeof(rrSchedule) / sizeof(rrSchedule[0]);

        for (int attempt = 0; attempt < rrSize; attempt++) {
            int idx = (serviceCounter + attempt) % rrSize;
            int q = rrSchedule[idx];

            if (!queues[q].empty()) {
                serviceCounter = (idx + 1) % rrSize;
                return q;
            }
        }
    } else {
        static const int weightedSchedule[] = {
            0, 0, 0, 0,
            1, 1, 1,
            2, 2,
            3,
            4
        };

        static const int weightedSize =
            sizeof(weightedSchedule) / sizeof(weightedSchedule[0]);

        for (int attempt = 0; attempt < weightedSize; attempt++) {
            int idx = (serviceCounter + attempt) % weightedSize;
            int q = weightedSchedule[idx];

            if (!queues[q].empty()) {
                serviceCounter = (idx + 1) % weightedSize;
                return q;
            }
        }
    }

    return -1;
}

LogEntry* MLFQScheduler::remove() {
    pthread_mutex_lock(&lock);

    while (count == 0) {
        pthread_cond_wait(&not_empty, &lock);
    }

    applyAgingBoost();
    int q = chooseQueue();

    QueuedLog ql = queues[q].front();
    queues[q].pop_front();
    LogEntry* item = ql.entry;
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