#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdatomic.h>
#include <time.h>
#include <signal.h>

#define BILLION 1000000000L

// Shared memory (global)
volatile atomic_int *shared_mem;

// Process argument structure
typedef struct {
    int id;
    long long read_count;
    int duration;
    int write_rate;
    int is_writer;
    int fill[10000];
} process_arg_t;

// Writer process function
void writer_process(process_arg_t* args) {
    int value = 0;
    while (1) {
        atomic_store(shared_mem, value++);
        printf(" %d",value);
        usleep(1000000 / args->write_rate); // Sleep for the given write rate
    }
}

// Reader process function
void reader_process(process_arg_t* args) {
    struct timespec start, end;
    long long sum = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        // Exit after the duration
        clock_gettime(CLOCK_MONOTONIC, &end);
        if ((end.tv_sec - start.tv_sec) >= args->duration) break;

        // Read from shared memory
        sum += atomic_load(shared_mem);
        args->read_count++;
    }
    printf("SUM %d: %lld\n", args->id, sum);
    exit(args->read_count); // Return the read count in the exit status
}

// Function to run the test
void run_test(int n_processes, int write_rate, int duration, int with_writer) {
    process_arg_t* process_args = malloc(n_processes * sizeof(process_arg_t));
    pid_t* pids = malloc(n_processes * sizeof(pid_t));

    // Start the processes
    for (int i = 0; i < n_processes; i++) {
        process_args[i].id = i;
        process_args[i].read_count = 0;
        process_args[i].duration = duration;
        process_args[i].write_rate = write_rate;
        process_args[i].is_writer = (i == 0 && with_writer);

        pids[i] = fork();
        if (pids[i] == 0) { // Child process
            if (process_args[i].is_writer) {
                writer_process(&process_args[i]);
            } else {
                reader_process(&process_args[i]);
            }
            exit(0);
        }
    }

    // Wait for the duration of the test
    sleep(duration);

    // Kill the writer process if it exists
    if (with_writer) {
        kill(pids[0], SIGKILL);
    }

    // Wait for all reader processes and gather results
    int total_reads = 0;
    for (int i = 1; i < n_processes; i++) {
        int status;
        waitpid(pids[i], &status, 0); // Wait for each reader process
        if (WIFEXITED(status)) {
            int read_count = WEXITSTATUS(status);
            printf("Reader %d: %d reads\n", i, read_count);
            total_reads += read_count;
        }
    }
    printf("Average reads per second: %.2f\n", (double)total_reads / duration);

    // Cleanup
    free(process_args);
    free(pids);
}

int main() {
    int n_processes, write_rate, duration;

    // Create shared memory
    int shm_fd = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(atomic_int));
    shared_mem = mmap(0, sizeof(atomic_int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Get user input
    printf("Enter the number of processes: ");
    scanf("%d", &n_processes);
    printf("Enter the write rate (writes per second) for the first process: ");
    scanf("%d", &write_rate);
    printf("Enter the duration of the test (seconds): ");
    scanf("%d", &duration);

    // Run the test with writing contention
    run_test(n_processes, write_rate, duration, 1);

    // Run the test without writing contention
    run_test(n_processes, write_rate, duration, 0);

    // Cleanup shared memory
    shm_unlink("/shared_mem");

    return 0;
}
