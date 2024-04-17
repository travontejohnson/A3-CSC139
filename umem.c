#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "umem.h"

#define HEADER_SIZE 32

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block;

static block *head = NULL;
static block *next_fit_ptr = NULL;
static int algorithm;
static size_t total_size;

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (head != NULL || sizeOfRegion <= 0) {
        return -1;
    }

    size_t adjustedSize = sizeOfRegion + HEADER_SIZE;

    void *ptr = mmap(NULL, adjustedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    head = (block *)ptr;
    // Adjusted for the space of the header
    head->size = sizeOfRegion - HEADER_SIZE;  
    head->free = 1;
    head->next = NULL;

    algorithm = allocationAlgo;
    total_size = sizeOfRegion;

    return 0;
}

void *umalloc(size_t size) {
    if (head == NULL || size == 0) {
        return NULL;
    }
    // This rounds to the nearest 8 
    size = (size + 7) / 8 * 8; 
    size += HEADER_SIZE;
    
    block *best = NULL;
    block *current = head;
    
    if (algorithm == BEST_FIT) {
        size_t min_size = total_size + 1;
        while (current) {
            if (current->free && current->size >= size && current->size < min_size) {
                best = current;
                min_size = current->size;
            }
            current = current->next;
        }
    } else if (algorithm == WORST_FIT) {
        size_t max_size = 0;
        while (current) {
            if (current->free && current->size >= size && current->size > max_size) {
                best = current;
                max_size = current->size;
            }
            current = current->next;
        }
    } else if (algorithm == FIRST_FIT) {
        while (current) {
            if (current->free && current->size >= size) {
                best = current;
                break;
            }
            current = current->next;
        }
    } else if (algorithm == NEXT_FIT) {
        if (!next_fit_ptr) {
            next_fit_ptr = head;
        }
        current = next_fit_ptr;
        while (current) {
            if (current->free && current->size >= size) {
                best = current;
                next_fit_ptr = current->next;
                break;
            }
            current = current->next;
            if (!current) {
                current = head;
            }
            if (current == next_fit_ptr) {
                break;
            }
        }
    }
    
    if (!best) {
        return NULL;
    }
    
    if (best->size >= size + HEADER_SIZE) {
        block *new_block = (block *)((char *)best + size);
        new_block->size = best->size - size;
        new_block->free = 1;
        new_block->next = best->next;
        best->size = size;
        best->next = new_block;
    }
    
    best->free = 0;
    return (void *)((char *)best + HEADER_SIZE);
}

int ufree(void *ptr) {
    if (!ptr) {
        return 0;
    }
    
    block *current = (block *)((char *)ptr - HEADER_SIZE);
    current->free = 1;
    
    // Coalesce with next block if free
    if (current->next && current->next->free) {
        current->size += current->next->size + HEADER_SIZE;
        current->next = current->next->next;
    }
    
    // Coalesce with previous block if free
    block *prev = head;
    while (prev && prev->next != current) {
        prev = prev->next;
    }
    if (prev && prev->free) {
        prev->size += current->size + HEADER_SIZE;
        prev->next = current->next;
    }
    
    return 0;
}

void umemdump() {
    block *current = head;
    printf("Memory Dump:\n");
    while (current) {
        printf("%p: %zu bytes (%s)\n", (void *)((char *)current + HEADER_SIZE), current->size - HEADER_SIZE, current->free ? "free" : "allocated");
        current = current->next;
    }
    printf("\n");
}