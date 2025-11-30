#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include "shared.h"
#include "logger.h"

void write_backing_store(int page_number) {
    printf("Writing page %d to backing store\n", page_number);
    FILE *fp = fopen("backing_store.txt", "a");
    if (!fp) {
        perror("fopen backing_store");
        return;
    }
    fprintf(fp, "Page %d: [0, 0, 0, 0]\n", page_number); // Simulate writing page as text
    fclose(fp);
}

int main() {
    srand(time(NULL));

    // Create shared memory
    key_t key = ftok("shared.h", 65);
    int shmid = shmget(key, sizeof(struct SharedMemory), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    struct SharedMemory *shm = (struct SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize shared memory
    shm->in = 0;
    shm->out = 0;
    shm->count = 0;
    shm->loaded_pages = 0;
    shm->fifo_front = 0;
    shm->fifo_rear = 0;
    shm->page_faults = 0;

    for (int i = 0; i < MAX_PAGES; i++) {
        shm->page_table[i].valid = 0;
        shm->page_table[i].frame_number = -1;
        shm->page_table[i].last_used = 0;
    }

    for (int i = 0; i < MAX_FRAMES; i++) {
        shm->frame_table[i].occupied = 0;
        shm->frame_table[i].page_number = -1;
        shm->fifo_queue[i] = 0;
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
        shm->buffer[i] = 0;
    }

    // Initialize semaphores
    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");

    sem_t *sem_empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
    sem_t *sem_full = sem_open("/sem_full", O_CREAT, 0644, 0);
    sem_t *sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    for (int i = 0; i < 25; i++) {
        int item = rand() % 100; // generates randoms numbers 

        sem_wait(sem_empty);
        sem_wait(sem_mutex);

        int index = shm->in;
        int page_number = index / PAGE_SIZE;

        if (page_number >= MAX_PAGES) {
            fprintf(stderr, "Page number %d out of bounds\n", page_number);
            exit(1);
        }

        if (!shm->page_table[page_number].valid) {
            shm->page_faults++;
            log_page_fault(page_number);
            printf("Producer: Page fault occurred for page %d (Total faults: %d)\n", page_number, shm->page_faults);

            if (shm->loaded_pages >= MAX_FRAMES) {
                int evict_index = shm->fifo_queue[shm->fifo_front];
                int evicted_page = shm->frame_table[evict_index].page_number;

                shm->page_table[evicted_page].valid = 0;
                shm->frame_table[evict_index].occupied = 0;

                log_page_replacement(evicted_page, page_number);
                write_backing_store(evicted_page);

                printf("Producer: Replaced page %d with page %d (FIFO)\n", evicted_page, page_number);

                shm->fifo_front = (shm->fifo_front + 1) % MAX_FRAMES;
            } else {
                shm->loaded_pages++;
            }

            int frame_number = shm->fifo_rear;
            shm->frame_table[frame_number].page_number = page_number;
            shm->frame_table[frame_number].occupied = 1;

            shm->page_table[page_number].frame_number = frame_number;
            shm->page_table[page_number].valid = 1;

            shm->fifo_queue[shm->fifo_rear] = frame_number;
            shm->fifo_rear = (shm->fifo_rear + 1) % MAX_FRAMES;

            printf("Producer: Loaded page %d into frame %d\n", page_number, frame_number);
            printf("Producer: Current loaded pages: %d\n", shm->loaded_pages);
        }

        shm->buffer[index] = item;
        shm->count++;
        log_produced(item, index);
        printf("Producer: Produced %d at index %d\n", item, index);

        shm->in = (shm->in + 1) % BUFFER_SIZE;
        log_buffer_state(shm->buffer, shm->count, shm->in, shm->out);

        sem_post(sem_mutex);
        sem_post(sem_full);

        usleep(500000);
    }

    // Cleanup
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(sem_empty);
    sem_close(sem_full);
    sem_close(sem_mutex);
    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");

    return 0;
}
