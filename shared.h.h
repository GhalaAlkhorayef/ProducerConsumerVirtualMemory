#ifndef SHARED_H
#define SHARED_H

#define BUFFER_SIZE 20
#define PAGE_SIZE 4
#define MAX_PAGES 10
#define MAX_FRAMES 3  // Reduced to 3 to trigger page replacement sooner

// Page table entry
typedef struct {
    int valid;        // 1 if page is in memory, 0 otherwise
    int frame_number; // Frame number in physical memory
    int last_used;    // For LRU (not used in FIFO)
} PageTableEntry;

// Frame table entry
typedef struct {
    int occupied;     // 1 if frame is occupied, 0 otherwise
    int page_number;  // Page number mapped to this frame
} FrameTableEntry;

// Shared memory structure
typedef struct SharedMemory {
    int buffer[BUFFER_SIZE];           // Circular buffer
    int in, out;                       // Indices for producer and consumer
    int count;                         // Number of items in the buffer
    PageTableEntry page_table[MAX_PAGES]; // Page table
    FrameTableEntry frame_table[MAX_FRAMES]; // Frame table
    int fifo_queue[MAX_FRAMES];        // FIFO queue for page replacement
    int fifo_front, fifo_rear;         // FIFO queue pointers
    int loaded_pages;                  // Number of pages currently loaded
    int page_faults;                   // Counter for page faults
} SharedMemory;

#endif

