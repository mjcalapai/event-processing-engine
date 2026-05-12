#include <fstream>
#include <iostream>
#include <string>

#include <pthread.h>
#include <list>
#include <atomic>

#include "log_entry.h"
#include "event_engine.h"
#include "producer.h"
#include "consumer.h"
#include "boundedBuffer.h"
#include "log_entry.h"

using namespace std;

int load_logs(char* filename); // forward declaration

std::list<LogEntry*> pendingLogs;
BoundedBuffer<LogEntry*>* bb = nullptr;

pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t process_lock = PTHREAD_MUTEX_INITIALIZER;

std::atomic<int> produced_count{0};
std::atomic<int> consumed_count{0};

void InitEventEngine(int p, int c, int size, char* filename) {
    produced_count = 0;
    consumed_count = 0;

    for (LogEntry* item : pendingLogs) {
        delete item;
    }
    pendingLogs.clear();

    bb = new BoundedBuffer<LogEntry*>(size);

    pthread_t* producers = new pthread_t[p];
    pthread_t* consumers = new pthread_t[c];

    int x = load_logs(filename);
    if (x != 0) {
        delete bb;
        delete[] producers;
        delete[] consumers;
        return;
    }

    int* producer_ids = new int[p];
    for (int i = 0; i < p; i++) {
        producer_ids[i] = i;
        pthread_create(&producers[i], nullptr, producer, &producer_ids[i]);
    }

    int* consumer_ids = new int[c];
    for (int j = 0; j < c; j++) {
        consumer_ids[j] = j;
        pthread_create(&consumers[j], nullptr, consumer, &consumer_ids[j]);
    }

    for (int i = 0; i < p; i++) {
        pthread_join(producers[i], nullptr);
    }

    for (int i = 0; i < c; i++) {
        bb->append(nullptr); // poison pill
    }

    for (int j = 0; j < c; j++) {
        pthread_join(consumers[j], nullptr);
    }

    delete[] producers;
    delete[] consumers;
    delete[] producer_ids;
    delete[] consumer_ids;
    delete bb;
}

int load_logs(char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return -1;
    }

    std::string line;
    std::string error;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        LogEntry* entry = new LogEntry();

        if (!parseLogLine(line, *entry, error)) {
            std::cerr << "Skipping malformed log line: " << error << std::endl;
            delete entry;
            continue;
        }

        pendingLogs.push_back(entry);
        produced_count++;
    }

    return 0;
}