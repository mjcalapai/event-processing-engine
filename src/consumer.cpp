#include "consumer.h"
#include "event_engine.h"
#include <iostream>

void handleDetection(LogEntry* item) { //basic, no correlation
    if (item->severity == Severity::CRITICAL) {
        std::cout << "[CRITICAL ALERT] " << item->toString() << std::endl;
        std::cout << "Recommended action: investigate immediately; consider temporary block or isolation.\n";
    }
    else if (item->severity == Severity::ERROR) {
        std::cout << "[ERROR ALERT] " << item->toString() << std::endl;
        std::cout << "Recommended action: flag for review; monitor for repeated behavior.\n";
    }
    else if (item->severity == Severity::WARNING) {
        std::cout << "[WARNING] " << item->toString() << std::endl;
    }
}

void* consumer(void*) {

    while (true) {

        LogEntry* item = bb->remove();

        if (item == nullptr) {
            break;
        }

        handleDetection(*item);

        pthread_mutex_lock(&process_lock);

        consumed_count++;

        pthread_mutex_unlock(&process_lock);

        delete item;
    }

    return nullptr;
}