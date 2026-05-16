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

    long long latencyNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            now - enqueueTime
        ).count();

    int idx = severityIndex(log->severity);

    processed++;
    totalLatencyNs += latencyNs;
    latencyBySeverity[idx] += latencyNs;
    countBySeverity[idx]++;

    if (latencyNs > maxLatencyBySeverity[idx]) {
        maxLatencyBySeverity[idx] = latencyNs;
    }
}

void Metrics::recordAlert() {
    alerts++;
}

void Metrics::recordAgingBoost() {
    agingBoosts++;
}

int Metrics::getAlerts() const {
    return alerts.load();
}

void Metrics::print(bool json) const {
    auto runtimeMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();

    double runtimeSec = runtimeMs / 1000.0;
    double throughput = runtimeSec > 0 ? processed / runtimeSec : 0.0;

    const char* names[5] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };

    if (json) {
        std::cout << "{\n";
        std::cout << "  \"runtimeMs\": " << runtimeMs << ",\n";
        std::cout << "  \"processed\": " << processed << ",\n";
        std::cout << "  \"throughput\": " << throughput << ",\n";
        std::cout << "  \"averageLatencyNs\": " << (processed > 0 ? totalLatencyNs / processed : 0) << ",\n";
        std::cout << "  \"latencyBySeverity\": {\n";
        bool first = true;
        for (int i = 0; i < 5; i++) {
            if (countBySeverity[i] == 0) continue;
            if (!first) std::cout << ",\n";
            std::cout << "    \"" << names[i] << "\": {";
            std::cout << "\"avg\": " << latencyBySeverity[i] / countBySeverity[i] << ", ";
            std::cout << "\"max\": " << maxLatencyBySeverity[i] << ", ";
            std::cout << "\"count\": " << countBySeverity[i];
            std::cout << "}";
            first = false;
        }
        std::cout << "\n  },\n";
        std::cout << "  \"alerts\": " << alerts << ",\n";
        std::cout << "  \"agingBoosts\": " << agingBoosts << "\n";
        std::cout << "}\n";
        return;
    }

    std::cout << "\n=== Metrics ===\n";
    std::cout << "Total runtime: " << runtimeMs << " ms\n";
    std::cout << "Processed events: " << processed << "\n";
    std::cout << "Throughput: " << throughput << " events/sec\n";

    if (processed > 0) {
        std::cout << "Average latency: "
                  << totalLatencyNs / processed
                  << " ns\n";
    }



    std::cout << "\nLatency by severity:\n";

    for (int i = 0; i < 5; i++) {
        if (countBySeverity[i] == 0) {
            continue;
        }

        std::cout << names[i]
                  << " avg=" << latencyBySeverity[i] / countBySeverity[i]
                  << " ns"
                  << ", max=" << maxLatencyBySeverity[i]
                  << " ns"
                  << ", count=" << countBySeverity[i]
                  << "\n";
    }

    std::cout << "\nAlerts: " << alerts << "\n";
    std::cout << "Aging boosts: " << agingBoosts << "\n";
}