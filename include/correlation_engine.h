#ifndef CORRELATION_ENGINE_H
#define CORRELATION_ENGINE_H

#include "log_entry.h"

#include <deque>
#include <unordered_map>
#include <string>
#include <pthread.h>

class CorrelationEngine {
public:
    CorrelationEngine();

    void process(const LogEntry* log);

private:
    struct EventRecord {
        Timestamp timestamp;
        std::string payload;
    };

    pthread_mutex_t lock;

    std::unordered_map<std::string, std::deque<EventRecord>> failedLoginsByIp;
    std::unordered_map<std::string, std::deque<EventRecord>> portScansByIp;

    const int failedLoginThreshold = 3;
    const int portScanThreshold = 5;
    const int windowSeconds = 60;

    bool containsCI(const std::string& haystack, const std::string& needle) const;

    void pruneOld(std::deque<EventRecord>& events, Timestamp now);
    void checkFailedLogin(const LogEntry* log);
    void checkPortScan(const LogEntry* log);
};

extern CorrelationEngine correlationEngine;

#endif