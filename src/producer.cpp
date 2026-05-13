#include <fstream>
#include "producer.h"
#include "severity_classifier.h"
#include "event_engine.h"
#include "log_entry.h"
#include "boundedBuffer.h"
using namespace std;



void* producer(void*) {
    SeverityClassifier classifier;

    while (true) {
        pthread_mutex_lock(&event_lock);

        if (pendingLogs.empty()) {
            pthread_mutex_unlock(&event_lock);
            break;
        }

        LogEntry* item = pendingLogs.front();
        pendingLogs.pop_front();

        pthread_mutex_unlock(&event_lock);

        Severity routedSeverity = classifier.classify(*item); //verify this could be a race condition low key
        item->severity = routedSeverity;


        if (activeMode == SchedulerMode::FIFO) {
            item->enqueueTime = std::chrono::steady_clock::now();
            bb->append(item);
        } else {
            item->enqueueTime = std::chrono::steady_clock::now(); //putting it on both conditional branches
            scheduler->append(item);                               // for closer to true evaluation
        }
    }

    return nullptr;
}