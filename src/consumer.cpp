#include "consumer.h"
#include "event_engine.h"

void* consumer(void*) {

    while (true) {

        LogEntry* item = bb->remove();

        if (item == nullptr) {
            break;
        }

        pthread_mutex_lock(&process_lock);

        consumed_count++;

        pthread_mutex_unlock(&process_lock);

        delete item;
    }

    return nullptr;
}