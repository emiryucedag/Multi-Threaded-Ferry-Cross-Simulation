#include <stdio.h>      // Standard Input/Output
#include <stdlib.h>     // General Utilities (malloc, rand, exit)
#include <unistd.h>     // Sleep, Usleep for delays
#include <pthread.h>    // Thread Operations
#include <semaphore.h>  // Semaphores for synchronization
#include <sys/time.h>   // High-precision time measurement
#include <errno.h>      // Error Codes
#include <stdbool.h>    // Boolean Type
#include <time.h>       // Time Functions
#include <fcntl.h>      // O_CREAT (Required for macOS sem_open compatibility)

// --- CONFIGURATION ---
#define FERRY_CAPACITY 5   // Maximum number of cars the ferry can carry
#define PROGRAM_RUNTIME 60 // Total duration of the simulation in seconds

// --- GLOBAL VARIABLES ---
// Mutex to protect critical sections where shared variables are modified
pthread_mutex_t car_count_mutex;

// Named semaphore pointers. 
//  use pointers and sem_open (instead of sem_init) to ensure compatibility 
// with macOS, which does not support unnamed semaphores fully.
sem_t *sem_board;    // Signals cars that they can board
sem_t *sem_full;     // Signals the ferry that the boat is full
sem_t *sem_unboard;  // Signals cars that they can unboard
sem_t *sem_empty;    // Signals the ferry that the boat is empty

int cars_on_board = 0;      // Shared counter for cars currently on the ferry
struct timeval start_time;  // Timestamp when the program started

// --- TIME FUNCTION ---
// Calculates the relative time elapsed since the start of the program.
// Returns the time in seconds with microsecond precision.
double get_relative_time_sec() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    double elapsed_sec = (current_time.tv_sec - start_time.tv_sec) + 
                         (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    return elapsed_sec;
}

// --- LOGGING FUNCTION ---
// Handles formatting and printing of simulation events.
// Includes a strict timing filter to meet the assignment requirement.
void print_status(const char* message, int car_num) {
    double current_time = get_relative_time_sec();

    // STRICT TIMING FILTER: 
    // If the simulation runs past the defined runtime (60s) due to cleanup operations, 
    // we suppress standard logs to ensure the output cuts off exactly as required.
    // (car_num -99 is reserved for special system messages if needed).
    if (current_time > PROGRAM_RUNTIME && car_num != -99) {
        return; 
    }

    if (car_num == -1 || car_num == -99) {
        // Ferry or System message
        printf("[Clock : %.4f] Ferry %s\n", current_time, message);
    } else {
        // Car message
        printf("[Clock : %.4f] Car %d %s\n", current_time, car_num, message);
    }
}

// --- FERRY THREAD ---
// Implements the Ferry logic: Boarding -> Crossing -> Unboarding -> Reset
void* ferry_thread(void* arg) {
    (void)arg; // Unused parameter
    print_status("arrives to new dock", -1);

    while (true) {
        // Check if the simulation time is up before starting a new cycle
        if (get_relative_time_sec() >= PROGRAM_RUNTIME) break;

        // 1. BOARDING PHASE
        // The ferry posts 'capacity' number of semaphores to allow cars to board.
        for (int i = 0; i < FERRY_CAPACITY; i++) {
            sem_post(sem_board);
        }

        // Wait until the 'sem_full' signal is received from the last boarding car.
        sem_wait(sem_full);

        // Check time again before departing to avoid starting a trip after time is up.
        if (get_relative_time_sec() >= PROGRAM_RUNTIME) break;

        // 2. CROSSING PHASE
        // Simulate travel time (3 Seconds)
        print_status("leaves the dock", -1);
        sleep(3); 

        // 3. UNBOARDING PHASE
        print_status("arrives to new dock", -1);
        // Signal permission for cars to unboard.
        for (int i = 0; i < FERRY_CAPACITY; i++) {
            sem_post(sem_unboard);
        }

        // Wait until the 'sem_empty' signal is received from the last leaving car.
        sem_wait(sem_empty);
    }
    return NULL;
}

// --- CAR THREAD ---
// Implements the Car logic: Queue -> Board -> Wait -> Unboard -> Random Wait
void* car_thread(void* arg) {
    int car_id = *(int*)arg;
    free(arg); // Free the memory allocated for the ID in main

    // Infinite loop: Cars loop continuously. They are not destroyed but 
    // cycle back to the queue, maintaining their IDs (1-5).
    while (true) {
        // Stop execution if time is up
        if (get_relative_time_sec() >= PROGRAM_RUNTIME) break;

        // --- 1. BOARDING PHASE ---
        // Wait for the ferry to signal boarding permission.
        sem_wait(sem_board); 

        // Critical Section: Incrementing car count
        pthread_mutex_lock(&car_count_mutex);
        
        // Simulate physical boarding time (10-50ms).
        // This prevents multiple threads from printing the exact same timestamp.
        usleep((rand() % 40000) + 10000); 

        cars_on_board++;
        print_status("entered the ferry", car_id);
        
        // If this is the last car to board (reaching capacity), signal the captain.
        if (cars_on_board == FERRY_CAPACITY) {
            sem_post(sem_full); 
        }
        pthread_mutex_unlock(&car_count_mutex);

        // --- 2. UNBOARDING PHASE ---
        // Wait for the ferry to reach the destination and signal unboarding.
        sem_wait(sem_unboard); 

        // Simulate physical unboarding time (5-25ms).
        usleep((rand() % 20000) + 5000); 
        print_status("left the ferry", car_id);

        // Critical Section: Decrementing car count
        pthread_mutex_lock(&car_count_mutex);
        cars_on_board--;
        
        // If this is the last car to leave (ferry is empty), signal the captain.
        if (cars_on_board == 0) {
            sem_post(sem_empty); 
        }
        pthread_mutex_unlock(&car_count_mutex);

        // --- 3. RETURN PHASE (Random Wait) ---
        // Simulate driving around the city before returning to the dock.
        // Wait between 0.5s and 1.5s.
        usleep((rand() % 1000000) + 500000);
    }
    return NULL;
}

int main() {
    pthread_t ferry_tid;
    pthread_t car_threads[FERRY_CAPACITY]; // Array to store thread IDs for proper cleanup

    srand(time(NULL));
    gettimeofday(&start_time, NULL);

    pthread_mutex_init(&car_count_mutex, NULL);

    // Initialize named semaphores. 
    // We unlink first to clean up any potential leftovers from previous runs.
    sem_unlink("/sem_board"); sem_unlink("/sem_full");
    sem_unlink("/sem_unboard"); sem_unlink("/sem_empty");

    // O_CREAT creates the semaphore if it doesn't exist.
    // 0644 gives read/write permissions to the owner.
    sem_board = sem_open("/sem_board", O_CREAT, 0644, 0);
    sem_full = sem_open("/sem_full", O_CREAT, 0644, 0);
    sem_unboard = sem_open("/sem_unboard", O_CREAT, 0644, 0);
    sem_empty = sem_open("/sem_empty", O_CREAT, 0644, 0);

    if (sem_board == SEM_FAILED) { perror("sem_open failed"); exit(EXIT_FAILURE); }

    // Create the Ferry Thread
    if (pthread_create(&ferry_tid, NULL, ferry_thread, NULL) != 0) {
        perror("Failed to create ferry thread"); exit(EXIT_FAILURE);
    }

    // Create Car Threads (Exactly 5 cars as per capacity)
    for (int i = 0; i < FERRY_CAPACITY; i++) {
        usleep((rand() % 999000) + 1000); // Random delay before creating each car

        int* car_id = (int*)malloc(sizeof(int));
        *car_id = i + 1; // Assign ID from 1 to 5

        if (pthread_create(&car_threads[i], NULL, car_thread, car_id) != 0) {
            perror("Failed to create car thread"); exit(EXIT_FAILURE);
        }
        
    }

    // --- MAIN EXECUTION CONTROL ---
    // The main thread sleeps for the exact duration of the program runtime.
    // This blocks the main thread while the simulation runs in the background.
    sleep(PROGRAM_RUNTIME);

    // --- TERMINATION PHASE ---
    // The simulation time is up. We need to stop all threads safely.
    
    // 1. Terminate and Join Ferry Thread
    // Since threads are in infinite loops, we send a cancellation request first.
    pthread_cancel(ferry_tid);
    pthread_join(ferry_tid, NULL);

    // 2. Terminate and Join Car Threads
    for (int i = 0; i < FERRY_CAPACITY; i++) {
        pthread_cancel(car_threads[i]);
        pthread_join(car_threads[i], NULL);
    }

    // --- CLEANUP ---
    // Destroy mutex and close/unlink semaphores to free system resources.
    pthread_mutex_destroy(&car_count_mutex);
    sem_close(sem_board); sem_unlink("/sem_board");
    sem_close(sem_full);  sem_unlink("/sem_full");
    sem_close(sem_unboard); sem_unlink("/sem_unboard");
    sem_close(sem_empty); sem_unlink("/sem_empty");

    return 0;
}