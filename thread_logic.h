#ifndef THREAD_LOGIC_H
#define THREAD_LOGIC_H

#include <pthread.h>
#include <stdbool.h>

typedef struct Train {
    int id;
    int direction;  // 0 for East, 1 for West
    int priority;   // 0 for low, 1 for high
    int arrival_time;
    int crossing_time;
    bool can_cross;
    bool has_crossed;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Train;

typedef struct Queue {
    Train *train;
    struct Queue *next;
} Queue;

// Package for dispatcher logic, used to return collection of data to thread_dispatcher
typedef struct StarvationResolution { 
    Train *train;
    Queue **source_queue;
} StarvationResolution;

void* train_thread(void *arg); 
double timespec_to_seconds(struct timespec *ts);
char* display_time(float time);

// Global variable declarations
extern pthread_mutex_t await_dispatcher;
extern pthread_mutex_t await_queue;
extern pthread_mutex_t await_cross;
extern pthread_cond_t ping_dispatcher;
extern pthread_cond_t ping_queue_ready;
extern pthread_cond_t ping_to_cross;
extern volatile int train_count;
extern volatile bool queue_ready;
extern volatile bool begin_simulation;
extern volatile int expected_trains;
extern Queue *low_priority;
extern Queue *high_priority;
extern struct timespec start_time;

#endif