ProducerConsumerVirtualMemory
Project Overview
This project is a simulation of the Producer–Consumer problem integrated with Virtual Memory management, developed for the CS330 – Introduction to Operating Systems course.
It demonstrates how concurrent processes interact with shared memory while applying paging, page replacement, and synchronization techniques commonly used in modern operating systems.
Objectives
Implement Producer–Consumer synchronization using semaphores
Simulate virtual memory with paging
Handle page faults and page replacement
Use inter-process communication (IPC) via shared memory
Log system behavior for debugging and analysis
Key Features
Producer–Consumer Synchronization
Bounded circular buffer (BUFFER_SIZE = 5)
POSIX semaphores:
sem_empty – tracks empty buffer slots
sem_full – tracks filled slots
sem_mutex – ensures mutual exclusion
Prevents race conditions and deadlocks
Virtual Memory & Paging
Memory divided into pages (PAGE_SIZE = 4)
Page table maintains virtual-to-physical memory mapping
Page faults occur when a requested page is not in memory
Page Replacement Policy
FIFO (First-In First-Out) algorithm
Oldest page is evicted when memory is full
Evicted pages are written to a file-based backing store
Inter-Process Communication (IPC)
Shared memory used to store:
Buffer
Page table
Frame data
Semaphores coordinate safe access between processes
Project Structure
ProducerConsumerVirtualMemory/
│
├── main.c              # Initializes shared memory and semaphores
├── producer.c          # Producer logic and page fault handling
├── consumer.c          # Consumer logic and page lookup
├── shared.h            # Shared structures and constants
├── logger.c / logger.h # Logging system
├── Makefile            # Compilation automation
├── logs.txt            # Execution logs
├── backing_store.txt   # Simulated disk storage
└── README.md
Logging & Output
Page faults, page replacements, and buffer activity are logged
Logs are written to logs.txt
Evicted pages are stored in backing_store.txt
Real-time output shows buffer state and memory usage
