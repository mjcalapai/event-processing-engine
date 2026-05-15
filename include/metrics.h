#ifndef METRICS_H
#define METRICS_H

#include "log_entry.h"
#include <chrono>
#include <atomic>

class Metrics {
public:
    void start();
    void stop();

    void recordProcessed(const LogEntry* log,
                         std::chrono::steady_clock::time_point enqueueTime);

    void recordAlert();
    void recordAgingBoost();

    int getAlerts() const;

    void print() const;

private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    std::atomic<int> processed{0};
    std::atomic<int> alerts{0};
    std::atomic<int> agingBoosts{0};

    long long totalLatencyNs = 0;
    long long latencyBySeverity[5] = {0, 0, 0, 0, 0};
    long long maxLatencyBySeverity[5] = {0, 0, 0, 0, 0};
    int countBySeverity[5] = {0, 0, 0, 0, 0};
};

extern Metrics metrics;

#endif