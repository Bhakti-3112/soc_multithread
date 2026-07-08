/*
 * parallel_sum_fixed.c
 *
 * Fixed version of the parallel sum program using a mutex to protect
 * the shared accumulator (total).
 *
 * Compile:
 *   gcc -Wall -pthread parallel_sum_fixed.c -o parallel_sum_fixed
 *
 * Run:
 *   ./parallel_sum_fixed
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ARRAY_SIZE  10000000
#define NUM_THREADS 4

int *array;                     /* heap-allocated, shared between threads */
long total = 0;                 /* shared accumulator */

/* Mutex to protect the shared total */
pthread_mutex_t total_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    long start_index;
    long end_index;             /* exclusive */
} thread_arg_t;

void *partial_sum(void *arg) {
    thread_arg_t *t = (thread_arg_t *)arg;

    for (long i = t->start_index; i < t->end_index; i++) {

        /* Enter critical section */
        pthread_mutex_lock(&total_lock);

        total += array[i];

        /* Leave critical section */
        pthread_mutex_unlock(&total_lock);
    }

    return NULL;
}

int main(void) {
    /* Allocate and initialize the array with 1s */
    array = malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        perror("malloc");
        return 1;
    }

    for (long i = 0; i < ARRAY_SIZE; i++) {
        array[i] = 1;
    }

    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    long chunk = ARRAY_SIZE / NUM_THREADS;

    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].start_index = i * chunk;
        args[i].end_index = (i == NUM_THREADS - 1)
                                ? ARRAY_SIZE
                                : (i + 1) * chunk;

        if (pthread_create(&threads[i], NULL, partial_sum, &args[i]) != 0) {
            perror("pthread_create");
            free(array);
            pthread_mutex_destroy(&total_lock);
            return 1;
        }
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Computed sum: %ld\n", total);
    printf("Expected:     %d\n", ARRAY_SIZE);

    if (total == ARRAY_SIZE) {
        printf("Correct! No race condition.\n");
    } else {
        printf("Off by %ld\n", (long)ARRAY_SIZE - total);
    }

    /* Clean up */
    pthread_mutex_destroy(&total_lock);
    free(array);

    return 0;
}
