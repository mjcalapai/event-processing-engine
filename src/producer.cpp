#include <fstream>
#include "producer.h"
#include "severity_classifier.h"
#include "log_entry.h"
#include "bounded_buffer.h"
using namespace std;

pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t process_lock = PTHREAD_MUTEX_INITIALIZER;




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