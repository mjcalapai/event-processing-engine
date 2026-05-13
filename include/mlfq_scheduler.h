#ifndef MLFQ_SCHEDULER_H
#define MLFQ_SCHEDULER_H

#include "log_entry.h"

#include <deque>
#include <pthread.h>
#include <chrono>

class MLFQScheduler {
public:
    explicit MLFQScheduler(int capacity);
    ~MLFQScheduler();

    void append(LogEntry* item);
    LogEntry* remove();

    bool isEmpty();

private:
    static const int NUM_QUEUES = 5;

    std::deque<LogEntry*> queues[NUM_QUEUES];

    int capacity;
    int count;

    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;

    int serviceCounter;

    int severityToQueue(Severity s);
    int chooseQueue();
};

#endif