#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* * Macro Definitions:
 * NUM_FILES: Total number of independent file operations to be performed.
 * THREAD_COUNT: Number of concurrent POSIX threads to be spawned.
 * LINES_PER_FILE: Number of write/read cycles within each file to increase I/O pressure.
 */
#define NUM_FILES 10000 
#define THREAD_COUNT 4
#define LINES_PER_FILE 1000

/*
 * Returns the current high-resolution monotonic time in seconds.
 * Used for precise execution duration measurement between execution blocks.
 */
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/*
 * Simulates a heavy I/O workload by triggering multiple system calls.
 * Workflow: Open -> Write Loop -> Rewind -> Read Loop -> Close -> Delete.
 * This function forces the OS to handle disk-bound operations and file system metadata.
 */
void perform_heavy_io(int id) {
    char filename[64];
    sprintf(filename, "io_test_%d.tmp", id);
    
    FILE *f = fopen(filename, "w+");
    if (!f) return;

    // Triggering write-intensive system calls
    for (int i = 0; i < LINES_PER_FILE; i++) {
        fprintf(f, "I/O System Call Generation - File ID: %d, Line: %d\n", id, i);
    }
    
    // Resetting file pointer to trigger read-intensive system calls
    rewind(f);
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f) != NULL);
    
    fclose(f);
    remove(filename); // Unlink the file to maintain storage hygiene
}

/*
 * Thread entry point. Divides the total file workload equally among threads.
 * Uses a range-based partitioning strategy to ensure each thread works on unique data.
 */
void* thread_routine(void* arg) {
    int thread_id = *(int*)arg;
    int files_per_thread = NUM_FILES / THREAD_COUNT;
    int start_idx = thread_id * files_per_thread;
    int end_idx = start_idx + files_per_thread;

    for (int i = start_idx; i < end_idx; i++) {
        perform_heavy_io(i);
    }
    return NULL;
}

int main() {
    printf("\n");
    printf("------------------------------------------\n");
    printf("[INFO] Workload: %d files | Lines: %d\n", NUM_FILES, LINES_PER_FILE);
    printf("------------------------------------------\n");

    // --- SEQUENTIAL EXECUTION BLOCK ---
    // Tasks are executed one after another in a single-threaded linear flow.
    printf("[PROCESS] Running Sequential Execution...\n");
    double start_s = get_time();
    for (int i = 0; i < NUM_FILES; i++) {
        perform_heavy_io(i);
    }
    double duration_s = get_time() - start_s;
    printf("[INFO] Sequential Duration: %.4f s\n\n", duration_s);

    // --- MULTI-THREADED EXECUTION BLOCK ---
    // Distributes tasks across multiple threads using the Pthreads library.
    printf("[PROCESS] Running Parallel Execution (using %d threads)...\n", THREAD_COUNT);
    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];
    
    double start_p = get_time();
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_routine, &thread_ids[i]);
    }

    // Barrier: Wait for all threads to finish their assigned I/O tasks.
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    double duration_p = get_time() - start_p;
    printf("[INFO] Parallel Duration: %.4f s\n", duration_p);

    return 0;
}