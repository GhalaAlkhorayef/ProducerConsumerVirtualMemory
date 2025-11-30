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
//swap out// 
void write_backing_store(int page_number) {
    printf("Writing page %d to backing store\n", page_number);
    FILE *fp = fopen("backing_store.txt", "a");
    if (!fp) {
        perror("fopen backing_store");
        return;
    }
    fprintf(fp, "Page %d: [0, 0, 0, 0]\n", page_number);
    fclose(fp);
}
//swap in // 
void read_backing_store(int page_number) {
    printf("Reading page %d from backing store\n", page_number);
    FILE *fp = fopen("backing_store.txt", "r");
    if (!fp) {
        perror("fopen backing_store");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int stored_page;
        if (sscanf(line, "Page %d:", &stored_page) == 1 && stored_page == page_number) {
            printf("Found page %d in backing store\n", page_number);
            break;
        }
    }
    fclose(fp);
}

int main() {
    srand(time(NULL));

    // Attach to shared memory
    key_t key = ftok("shared.h", 65);
    int shmid = shmget(key, sizeof(struct SharedMemory), 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    struct SharedMemory *shm = (struct SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Open semaphores
    sem_t *sem_empty = sem_open("/sem_empty", 0);
    sem_t *sem_full = sem_open("/sem_full", 0);
    sem_t *sem_mutex = sem_open("/sem_mutex", 0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED || sem_mutex == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    for (int i = 0; i < 25; i++) {
        sem_wait(sem_full); // checking if there is an items toconsum and then lock so no other process accses it
        sem_wait(sem_mutex);

        int index = shm->out;
        int page_number = index / PAGE_SIZE; // to find where to read from 

        if (page_number >= MAX_PAGES) {
            fprintf(stderr, "Page number %d out of bounds\n", page_number);
            exit(1);
        }

        if (!shm->page_table[page_number].valid) { // if = 0 then page fault occure 
            shm->page_faults++;
            log_page_fault(page_number);
            printf("Consumer: Page fault occurred for page %d (Total faults: %d)\n", page_number, shm->page_faults);

            if (shm->loaded_pages >= MAX_FRAMES) {
                int evict_index = shm->fifo_queue[shm->fifo_front];
                int evicted_page = shm->frame_table[evict_index].page_number;

                shm->page_table[evicted_page].valid = 0;
                shm->frame_table[evict_index].occupied = 0;

                log_page_replacement(evicted_page, page_number);
                write_backing_store(evicted_page);

                printf("Consumer: Replaced page %d with page %d (FIFO)\n", evicted_page, page_number);

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

            read_backing_store(page_number);
            printf("Consumer: Loaded page %d into frame %d\n", page_number, frame_number);
            printf("Consumer: Current loaded pages: %d\n", shm->loaded_pages);
        }

        int item = shm->buffer[index];
        shm->count--;
        log_consumed(item, index); // now theconsumer remove the iteam 
        printf("Consumer: Consumed %d from index %d\n", item, index);

        shm->out = (shm->out + 1) % BUFFER_SIZE;
        log_buffer_state(shm->buffer, shm->count, shm->in, shm->out);

        sem_post(sem_mutex); // unlock 
        sem_post(sem_empty);

        sleep(1);
    }

    // Cleanup
    shmdt(shm);

    return 0;
}
