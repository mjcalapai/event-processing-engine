#include "correlation_engine.h"
#include "metrics.h"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>

CorrelationEngine correlationEngine;

CorrelationEngine::CorrelationEngine() {
    pthread_mutex_init(&lock, nullptr);
}

bool CorrelationEngine::containsCI(const std::string& haystack,
                                   const std::string& needle) const {
    if (needle.empty()) {
        return false;
    }

    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower(a) == std::tolower(b);
        }
    );

    return it != haystack.end();
}

void CorrelationEngine::pruneOld(std::deque<EventRecord>& events, Timestamp now) {
    while (!events.empty()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - events.front().timestamp
        ).count();

        if (age <= windowSeconds) {
            break;
        }

        events.pop_front();
    }
}

void CorrelationEngine::process(const LogEntry* log) {
    pthread_mutex_lock(&lock);

    checkFailedLogin(log);
    checkPortScan(log);

    pthread_mutex_unlock(&lock);
}

void CorrelationEngine::checkFailedLogin(const LogEntry* log) {
    if (log->type != LogType::AUTH) {
        return;
    }

    if (!containsCI(log->payload, "failed login")) {
        return;
    }

    auto& events = failedLoginsByIp[log->source_ip];

    events.push_back({log->timestamp, log->payload});
    pruneOld(events, log->timestamp);

    if (static_cast<int>(events.size()) == failedLoginThreshold) {
        std::cerr << "[CORRELATED ALERT] Possible brute-force login attempt from "
                  << log->source_ip
                  << " — "
                  << events.size()
                  << " failed logins within "
                  << windowSeconds
                  << " seconds."
                  << std::endl;

        metrics.recordAlert();
    }
}

void CorrelationEngine::checkPortScan(const LogEntry* log) {
    if (log->type != LogType::NETWORK) {
        return;
    }

    if (!containsCI(log->payload, "port")) {
        return;
    }

    if (!containsCI(log->payload, "connection attempt") &&
        !containsCI(log->payload, "packet dropped")) {
        return;
    }

    auto& events = portScansByIp[log->source_ip];

    events.push_back({log->timestamp, log->payload});
    pruneOld(events, log->timestamp);

    if (static_cast<int>(events.size()) == portScanThreshold) {
        std::cerr << "[CORRELATED ALERT] Possible port scan from "
                  << log->source_ip
                  << " — "
                  << events.size()
                  << " network probe events within "
                  << windowSeconds
                  << " seconds."
                  << std::endl;

        metrics.recordAlert();
    }
}