#include "mem.h"
#define MBS 16
extern BLOCK_HEADER* first_header;

int isAlloc(BLOCK_HEADER* h) {
    if (h->size_alloc & 1)
        return 1;           // is allocated (not free)
    return 0;               // is not allocated (free)
}

BLOCK_HEADER* getNextHeader(BLOCK_HEADER* h) {
    return (BLOCK_HEADER*)((unsigned long)(h) + (h->size_alloc & 0xFFFFFFFE));
}

// return a pointer to the payload
// if a large enough free block isn't available, return NULL
void* Mem_Alloc(int size){
    BLOCK_HEADER* curr = first_header;  // Initialize pointer for traversal

    // Set block size
    int blockSize = size + 8;  // Payload + Header
    if (blockSize % 16)
        blockSize += (16 - (blockSize % 16)); // + Padding

    // Traverse memory, skipping all blocks that are allocated or too small, and exiting loop if last header is reached
    while ((isAlloc(curr) || curr->size_alloc < blockSize) && curr->size_alloc != 1)
        curr = getNextHeader(curr);

    // If last header is reached, return NULL
    if (curr->size_alloc == 1)
        return NULL;

    int currSize = curr->size_alloc & 0xFFFFFFFE;                       // Store size of current block
    curr->size_alloc += 1;                                              // Add alloc bit
    curr->payload = size;                                               // Set payload size
    int padding = (curr->size_alloc & 0xFFFFFFFE) - curr->payload - 8;  // Store padding for alignment

    // Split if necessary (if padding size is greater than or equal to 16 split the block)
    if (padding >= 16) {
        curr->size_alloc = blockSize + 1;

        BLOCK_HEADER* newBlock = getNextHeader(curr);  // Set newBlock to first memory location after current block
        newBlock->size_alloc = currSize - blockSize;   // Size_alloc is remaining bytes from original current block
        newBlock->payload = newBlock->size_alloc - 8;  // Payload is every byte in size_alloc, except 8 for header
    }
    return (BLOCK_HEADER*)((unsigned long)(curr) + 8);
}

// return 0 on success
// return -1 if the input ptr was invalid
int Mem_Free(void *ptr) {
    BLOCK_HEADER* curr = first_header;                   // Initialize header for traversal
    void* currPtr = (void*)((unsigned long)(curr) + 8);  // Initialize first user pointer
    BLOCK_HEADER* prev = NULL;                           // Prev will be used in the case that block before ptr is free, for coalescing

    // If ptr is not at the start of memory, initialize prev
    if (currPtr != ptr) {
        curr = getNextHeader(curr);                      // Advance pointer
        currPtr = (void*)((unsigned long)(curr) + 8);    // Advance pointer
        prev = first_header;                             // Initialize prev as block before curr in memory
    }

    // Traverse memory
    while (currPtr != ptr && curr->size_alloc != 1) {
        curr = getNextHeader(curr);
        currPtr = (void*)((unsigned long)(curr) + 8);
        prev = getNextHeader(prev);
    }

    // if you reach the end of the list without finding it return -1
    if (curr->size_alloc == 1)
        return -1;

    // If ptr found and is alloc, free curr
    if (currPtr == ptr && curr->size_alloc % 2) {
        curr->size_alloc -= 1;

        // coalesce adjacent free blocks
        if (prev != NULL && !isAlloc(prev)) {
            prev->size_alloc += curr->size_alloc;
            prev->payload - curr->size_alloc - 8;
            curr = prev;
        }
        BLOCK_HEADER* next = getNextHeader(curr);
        if (!isAlloc(next)) {
            curr->size_alloc += next->size_alloc;
            curr->payload = curr->size_alloc - 8;
        }
    } else  // ptr not found
        return -1;
    return 0;
}
