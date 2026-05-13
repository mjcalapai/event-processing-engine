#ifndef EVENT_ENGINE_H
#define EVENT_ENGINE_H

#include "log_entry.h"
#include "boundedBuffer.h"
#include "mlfq_scheduler.h"


#include <list>
#include <pthread.h>
#include <atomic>

extern std::list<LogEntry*> pendingLogs;

extern pthread_mutex_t event_lock;
extern pthread_mutex_t process_lock;

enum class SchedulerMode {
    FIFO,
    WEIGHTED
};

extern BoundedBuffer<LogEntry*>* bb;
extern MLFQScheduler* scheduler;
extern SchedulerMode activeMode;

extern std::atomic<int> produced_count;
extern std::atomic<int> consumed_count;

void InitEventEngine(SchedulerMode mode, int p, int c, int size, char* filename);

int load_logs(char* filename);

#endif