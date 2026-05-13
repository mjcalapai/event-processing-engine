#ifndef MLFQ_SCHEDULER_H
#define MLFQ_SCHEDULER_H

#include "log_entry.h"

#include <deque>
#include <pthread.h>
#include <chrono>

struct QueuedLog {
    LogEntry* entry;

    std::chrono::steady_clock::time_point enqueueTime;
};

enum class MLFQPolicy {
    RR,
    WEIGHTED
};

class MLFQScheduler {
public:
    explicit MLFQScheduler(int capacity, MLFQPolicy policy);
    ~MLFQScheduler();

    void append(LogEntry* item);
    LogEntry* remove();

    bool isEmpty();

private:
    static const int NUM_QUEUES = 5;

    std::deque<QueuedLog> queues[NUM_QUEUES];

    int capacity;
    int count;

    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;

    int serviceCounter;

    int severityToQueue(Severity s);
    int chooseQueue();
    void applyAgingBoost();

    int agingThresholdSeconds[NUM_QUEUES] = {
        0, // CRITICAL
        8, // ERROR
        6, // WARNING
        4, // INFO
        3  // DEBUG
    };
    MLFQPolicy policy;
};

#endif