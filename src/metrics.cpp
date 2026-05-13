#include "metrics.h"
#include <iostream>

Metrics metrics;

static int severityIndex(Severity s) {
    return static_cast<int>(s);
}

void Metrics::start() {
    startTime = std::chrono::steady_clock::now();
}

void Metrics::stop() {
    endTime = std::chrono::steady_clock::now();
}

void Metrics::recordProcessed(const LogEntry* log,
                              std::chrono::steady_clock::time_point enqueueTime) {
    auto now = std::chrono::steady_clock::now();

    long long latencyMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - enqueueTime
        ).count();

    int idx = severityIndex(log->severity);

    processed++;
    totalLatencyMs += latencyMs;
    latencyBySeverity[idx] += latencyMs;
    countBySeverity[idx]++;

    if (latencyMs > maxLatencyBySeverity[idx]) {
        maxLatencyBySeverity[idx] = latencyMs;
    }
}

void Metrics::recordAlert() {
    alerts++;
}

void Metrics::recordAgingBoost() {
    agingBoosts++;
}

void Metrics::print() const {
    auto runtimeMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();

    double runtimeSec = runtimeMs / 1000.0;
    double throughput = runtimeSec > 0 ? processed / runtimeSec : 0.0;

    std::cout << "\n=== Metrics ===\n";
    std::cout << "Total runtime: " << runtimeMs << " ms\n";
    std::cout << "Processed events: " << processed << "\n";
    std::cout << "Throughput: " << throughput << " events/sec\n";

    if (processed > 0) {
        std::cout << "Average latency: "
                  << totalLatencyMs / processed
                  << " ms\n";
    }

    const char* names[5] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };

    std::cout << "\nLatency by severity:\n";

    for (int i = 0; i < 5; i++) {
        if (countBySeverity[i] == 0) {
            continue;
        }

        std::cout << names[i]
                  << " avg=" << latencyBySeverity[i] / countBySeverity[i]
                  << " ms"
                  << ", max=" << maxLatencyBySeverity[i]
                  << " ms"
                  << ", count=" << countBySeverity[i]
                  << "\n";
    }

    std::cout << "\nAlerts: " << alerts << "\n";
    std::cout << "Aging boosts: " << agingBoosts << "\n";
}