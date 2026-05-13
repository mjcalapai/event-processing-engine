#include "consumer.h"
#include "event_engine.h"
#include "metrics.h"
#include "correlation_engine.h"
#include <iostream>

void handleDetection(LogEntry* item) {
    if (item->severity == Severity::CRITICAL) {
        std::cout << "[CRITICAL ALERT] " << item->toString() << std::endl;
        metrics.recordAlert();
    }
    else if (item->severity == Severity::ERROR) {
        std::cout << "[ERROR ALERT] " << item->toString() << std::endl;
        metrics.recordAlert();
    }
}

void* consumer(void*) {
    while (true) {
        LogEntry* item = nullptr;

        if (activeMode == SchedulerMode::FIFO) {
            item = bb->remove();
        } else {
            item = scheduler->remove();
        }

        if (item == nullptr) {
            break;
        }

        handleDetection(item);
        correlationEngine.process(item);
        metrics.recordProcessed(item, item->enqueueTime);

        pthread_mutex_lock(&process_lock);
        consumed_count++;
        pthread_mutex_unlock(&process_lock);

        delete item;
    }

    return nullptr;
}