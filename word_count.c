/*
 * word_count.c
 *
 * Parallel word count using pthreads.
 *
 * Compile:
 * gcc -Wall -pthread word_count.c -o word_count
 *
 * Run:
 * ./word_count <filename> <num_threads>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

typedef struct {
    char *buffer;
    long start;
    long end;
} ThreadData;

long total_words = 0;

void *count_words(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    long local_count = 0;
    int inside_word = 0;

    long i = data->start;

    if (i > 0 && !isspace((unsigned char)data->buffer[i - 1])) {
        while (i < data->end &&
               !isspace((unsigned char)data->buffer[i])) {
            i++;
        }
    }

    for (; i < data->end; i++) {

        if (isspace((unsigned char)data->buffer[i])) {
            inside_word = 0;
        } else {
            if (!inside_word) {
                local_count++;
                inside_word = 1;
            }
        }
    }

    total_words += local_count;

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <filename> <num_threads>\n",
                argv[0]);
        return 1;
    }

    char *filename = argv[1];
    int num_threads = atoi(argv[2]);

    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive.\n");
        return 1;
    }

    FILE *fp = fopen(filename, "rb");

    if (!fp) {
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *buffer = malloc(file_size + 1);

    if (!buffer) {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, file_size, fp);

    fclose(fp);

    buffer[bytes_read] = '\0';

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *data = malloc(num_threads * sizeof(ThreadData));

    if (!threads || !data) {
        perror("malloc");
        free(buffer);
        free(threads);
        free(data);
        return 1;
    }

    long chunk = file_size / num_threads;

    for (int i = 0; i < num_threads; i++) {

        data[i].buffer = buffer;
        data[i].start = i * chunk;

        if (i == num_threads - 1)
            data[i].end = file_size;
        else
            data[i].end = (i + 1) * chunk;

        pthread_create(&threads[i],
                       NULL,
                       count_words,
                       &data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Total words = %ld\n", total_words);

    free(buffer);
    free(threads);
    free(data);

    return 0;
}
