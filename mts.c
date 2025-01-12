#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "thread_logic.h"

#define NANOSECOND_CONVERSION 1e9

typedef struct Queue Queue;
typedef struct Train Train;
typedef struct DispacterLogicPackage DispacterLogicPackage;

pthread_mutex_t await_dispatcher = PTHREAD_MUTEX_INITIALIZER; // Broadcast mutex
pthread_mutex_t await_queue = PTHREAD_MUTEX_INITIALIZER; // Queue mutex
pthread_mutex_t await_cross = PTHREAD_MUTEX_INITIALIZER; // Crossing mutex
pthread_cond_t ping_dispatcher = PTHREAD_COND_INITIALIZER; // Broadcast signal
pthread_cond_t ping_queue_ready = PTHREAD_COND_INITIALIZER; // Queue signal
pthread_cond_t ping_to_cross = PTHREAD_COND_INITIALIZER; // Crossing signal

volatile int train_count = 0; // Number of trains
volatile bool queue_ready = true; // Queue condition
volatile bool begin_simulation = false; // Broadcast condition
volatile int expected_trains = 0; // Expected number of trains

Queue *low_priority = NULL;
Queue *high_priority = NULL;
struct timespec start_time = { 0 };

StarvationResolution resolve_starvation(const char* last_direction) {
    // When one directions sends multiple trains, check to see if other direction can send another
    // Returns such a train, if it exists
    StarvationResolution result = {NULL, NULL};
    int prev_direction = (strcmp(last_direction, "WEST") == 0) ? 1 : 0; 
    
    if(high_priority != NULL) {
        Queue *ptr = high_priority;
        Queue **current = &high_priority;
        while(ptr != NULL) {
            if(ptr->train->direction != prev_direction) {
                result.train = ptr->train;
                result.source_queue = current;
                return result;
            }
            current = &(ptr->next);
            ptr = ptr->next;
        }
    }
    if(low_priority != NULL) {
        Queue *ptr = low_priority;
        Queue **current = &low_priority;
        while(ptr != NULL) {
            if(ptr->train->direction != prev_direction) {
                result.train = ptr->train;
                result.source_queue = current;
                return result;
            }
            current = &(ptr->next);
            ptr = ptr->next;
        }
    }
    return result;
}
int update_same_direction(const char* last_direction, Train *train_to_cross, int current_same_direction){
    // Keep track of number of consecutive trains in the same direction
    char* current_direction = train_to_cross->direction == 1 ? "WEST" : "EAST";
    if (strcmp(last_direction, current_direction) != 0 || strcmp(last_direction, "NONE") == 0){
        return 1;
    }
    return current_same_direction + 1;
}

Train* thread_dispatcher(Queue **queue, char **prev_direction, int *same_direction_count){
    // Main loop logic for dispatching trains
    if (queue == NULL){
        return NULL;
    }
    Train *train_to_cross = NULL;
    Train *head_train = (*queue)->train;
    Queue **queue_to_access = queue;

   if (*same_direction_count >= 2) {
        // Resolve starvation when detected
        StarvationResolution resolution = resolve_starvation(*prev_direction);
        if(resolution.train != NULL) {
            train_to_cross = resolution.train;
            queue_to_access = resolution.source_queue;
            *same_direction_count = 1;
        }
    }
    if(train_to_cross == NULL){
        // Enter if starvation is not detected
        if ((*queue)->next == NULL){
            // Enter if only one train in queue
            Queue *ptr = *queue;
            *queue = NULL;
            *same_direction_count = update_same_direction(*prev_direction, head_train, *same_direction_count);
            *prev_direction = (head_train->direction == 1) ? "WEST" : "EAST";
            free(ptr);
            return head_train;
        }
        Train *following_train = (*queue)->next->train;
        
        if(head_train->direction == following_train->direction){
            // Both travelling in the same direction cases
            if(head_train->arrival_time != following_train->arrival_time){
                // Different arrival times
                train_to_cross = head_train->arrival_time < following_train->arrival_time ? head_train : following_train;
                queue_to_access = head_train->arrival_time < following_train->arrival_time ? queue : &(*queue)->next;
            }
            else{
                    // Same arrivatl times, decide by when arrived
                    train_to_cross = head_train->id < following_train->id ? head_train : following_train;
                    queue_to_access = head_train->id < following_train->id ? queue : &(*queue)->next;
                }
        }
        else{
            // Trains are travelling in different directions
            if(strcmp(*prev_direction, "EAST") == 0){
                // Train with opposite direction to last cross crosses, other West trains cross
                train_to_cross = head_train->direction == 0 ? head_train : following_train;
                queue_to_access = head_train->direction == 0 ? queue : &(*queue)->next;
                *prev_direction = "WEST";
            }
            else{
                // Train with opposite direction to last cross crosses, other East trains cross
                train_to_cross = head_train->direction == 1 ? head_train : following_train;
                queue_to_access = head_train->direction == 1 ? queue : &(*queue)->next;
                *prev_direction = "EAST";
            }
        }
    }
    if(train_to_cross != NULL){
        // Update queue pointer, what train is going to cross, and update same direction count
        Queue *ptr = *queue_to_access;
        *queue_to_access = (*queue_to_access)->next;
        *same_direction_count = update_same_direction(*prev_direction, train_to_cross, *same_direction_count);
        *prev_direction = (train_to_cross->direction == 1) ? "WEST" : "EAST";
        free(ptr);
    }
    return train_to_cross;
}

int main(int argc, char *argv[]) {   
    if (argc != 2){
        printf("USAGE: ./mts <input_file.txt>\n");
        return 1;
    }
    char* filename = argv[1];
    FILE *input = fopen(filename, "r");
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), input) != NULL) {
        expected_trains++;
    }
    rewind(input);
    pthread_mutex_init(&await_dispatcher, NULL);
    pthread_cond_init(&ping_dispatcher, NULL);
    pthread_t t_threads[100];  // Array to store thread IDs
    int train_id = 0;
    while (fgets(buffer, sizeof(buffer), input) != NULL) {   
        // Parse train data
        Train *train = (Train*)malloc(sizeof(Train));
        train->id = train_id;
        
        char* token = strtok(buffer, " ");
        if (token[0] == 'e' || token[0] == 'E') {
            train->direction = 0; 
            train->priority = (token[0] == 'E') ? 1 : 0;
        } else if (token[0] == 'w' || token[0] == 'W') {
            train->direction = 1; 
            train->priority = (token[0] == 'W') ? 1 : 0;
        }
        
        token = strtok(NULL, " ");
        train->arrival_time = atoi(token);
        token = strtok(NULL, " ");
        train->crossing_time = atoi(token);
        train->can_cross = 0;
        pthread_mutex_init(&train->mutex, NULL);
        pthread_cond_init(&train->cond, NULL);
                
        pthread_create(&t_threads[train_id], NULL, train_thread, (void*)train);
        train_id++;
    }
    fclose(input);
    
    freopen("output.txt", "w", stdout);

    // Signal all threads to begin the simulation
    sleep(1);
    pthread_mutex_lock(&await_dispatcher);
    while(train_count < expected_trains){
        pthread_cond_wait(&ping_dispatcher, &await_dispatcher);
    }
    pthread_cond_broadcast(&ping_dispatcher);
    begin_simulation = true;
    pthread_mutex_unlock(&await_dispatcher);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    char* last_direction = "NONE";
    int same_direction_count = 0;
    while(train_count > 0){
        // Loop dispacter logic until all trains have been crossed
        Train *train_to_cross = NULL;
        if(high_priority != NULL){
            train_to_cross = thread_dispatcher(&high_priority, &last_direction, &same_direction_count);
        }
        else if (low_priority != NULL){
            train_to_cross = thread_dispatcher(&low_priority, &last_direction, &same_direction_count);
        }
        if(train_to_cross != NULL){
            // When train is selected to cross, signal it to cross
            pthread_mutex_lock(&train_to_cross->mutex);
            train_to_cross->can_cross = 1;
            pthread_cond_signal(&train_to_cross->cond);
            pthread_mutex_unlock(&train_to_cross->mutex);
            
            // Wait until train has crossed
            pthread_mutex_lock(&await_cross);
            while(!train_to_cross->has_crossed){
                pthread_cond_wait(&ping_to_cross, &await_cross);
            }
            pthread_mutex_unlock(&await_cross);
        }
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < train_count; i++) {
        pthread_join(t_threads[i], NULL);
    }

    // Cleanup
    pthread_mutex_destroy(&await_dispatcher);
    pthread_cond_destroy(&ping_dispatcher);
    pthread_mutex_destroy(&await_queue);
    pthread_cond_destroy(&ping_queue_ready);
    pthread_mutex_destroy(&await_cross);
    pthread_cond_destroy(&ping_to_cross);
    
    fclose(stdout);
    return 0;
}

