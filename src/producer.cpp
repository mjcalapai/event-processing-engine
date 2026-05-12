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

        bb->append(item);
    }

    return nullptr;
}