#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "thread_logic.h"

#define NANOSECOND_CONVERSION 1e9

double timespec_to_seconds(struct timespec *ts)
{
	return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / NANOSECOND_CONVERSION);
}

char* display_time(float time) {
    static char result[24];
    int tenth = (int)(time * 10); 
    int hours = tenth / (3600);
    int minutes = (tenth % (3600)) / (600);
    float seconds = (float)tenth / 10.0;
    seconds = seconds - (hours * 3600) - (minutes * 600);
    sprintf(result, "%02d:%02d:%04.1f", hours, minutes, seconds);
    return result;
}
void* train_thread(void *arg) {
    // Thread logic
    Train *train = (Train*)arg;
    char* current_direction = (train->direction == 1) ? "West" : "East";

    // Wait until signaled to start simulation
    train_count++;
    pthread_mutex_lock(&await_dispatcher);
    while (!begin_simulation) {
        pthread_cond_wait(&ping_dispatcher, &await_dispatcher);
    }
    pthread_mutex_unlock(&await_dispatcher);

    // Wait until arrival time
    struct timespec at = {
        .tv_sec = train->arrival_time / 10,
        .tv_nsec = (train->arrival_time % 10) * 1e8  
    };
    nanosleep(&at, NULL); 

    struct timespec load_time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &load_time);
    char *time_display = display_time(timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    printf("%s Train %d is ready to go %s\n", time_display, train->id, current_direction);
    
    // Wait until queue access is uncontested by other threads
    pthread_mutex_lock(&await_queue);
    while (!queue_ready) {
        pthread_cond_wait(&ping_queue_ready, &await_queue);
    }
    queue_ready = false;

    Queue *new_node = (Queue*)malloc(sizeof(Queue));
    new_node->train = train;
    new_node->next = NULL;

    if (train->priority == 1) {
        if (high_priority == NULL) {
            high_priority = new_node;
        } else {
            Queue *temp = high_priority;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = new_node;
        }
    } else {
        if (low_priority == NULL) {
            low_priority = new_node;
        } else {
            Queue *temp = low_priority;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = new_node;
        }
    }
    // Release queue access
    queue_ready = true;
    pthread_cond_signal(&ping_queue_ready);
    pthread_mutex_unlock(&await_queue);

    // Wait until signaled to cross by dispatcher
    pthread_mutex_lock(&train->mutex);
    while (!train->can_cross) {
        pthread_cond_wait(&train->cond, &train->mutex);
    }
    pthread_mutex_unlock(&train->mutex);

    // Train is crossing
    pthread_mutex_lock(&await_cross);
    clock_gettime(CLOCK_MONOTONIC, &load_time);
    time_display = display_time(timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    printf("%s Train %d is ON the main track going %s\n", time_display, 
        train->id, current_direction);

    struct timespec ct = {
        .tv_sec = train->crossing_time / 10,
        .tv_nsec = (train->crossing_time % 10) * 1e8
    };
    nanosleep(&ct, NULL);
    
    clock_gettime(CLOCK_MONOTONIC, &load_time);
    time_display = display_time(timespec_to_seconds(&load_time) - timespec_to_seconds(&start_time));
    printf("%s Train %d is OFF the main track going %s\n", time_display, 
        train->id, current_direction);

    // Signal that train has crossed to dispatcher
    train->has_crossed = true;
    pthread_cond_signal(&ping_to_cross);
    pthread_mutex_unlock(&await_cross);

    // Cleanup
    train_count--;
    return NULL;
}