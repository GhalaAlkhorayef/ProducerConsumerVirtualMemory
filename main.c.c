#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include "shared.h"

int main() {
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

    // Fork producer and consumer processes
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: Producer
        execl("./producer", "producer", NULL);
        perror("execl producer");
        exit(1);
    }

    pid = fork();
    if (pid == 0) {
        // Child process: Consumer
        execl("./consumer", "consumer", NULL);
        perror("execl consumer");
        exit(1);
    }

    // Wait for children to finish
    for (int i = 0; i < 2; i++) {
        wait(NULL);
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
