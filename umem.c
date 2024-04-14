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
static int algo;
static size_t total_size;

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (head != NULL || sizeOfRegion <= 0) {
        return -1;
    }
    
    size_t page_size = getpagesize();
    total_size = (sizeOfRegion + page_size - 1) / page_size * page_size;
    
    int fd = open("/dev/zero", O_RDWR);
    void *ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    head = (block *)ptr;
    head->size = total_size - HEADER_SIZE;
    head->free = 1;
    head->next = NULL;
    
    algo = allocationAlgo;
    
    return 0;
}

void *umalloc(size_t size) {
    if (head == NULL) {
        return NULL;
    }
    
    size = (size + 7) / 8 * 8; // Round up to nearest multiple of 8
    size += HEADER_SIZE;
    
    block *best = NULL;
    block *current = head;
    
    if (algo == BEST_FIT) {
        size_t min_size = total_size + 1;
        while (current) {
            if (current->free && current->size >= size && current->size < min_size) {
                best = current;
                min_size = current->size;
            }
            current = current->next;
        }
    } else if (algo == WORST_FIT) {
        size_t max_size = 0;
        while (current) {
            if (current->free && current->size >= size && current->size > max_size) {
                best = current;
                max_size = current->size;
            }
            current = current->next;
        }
    } else if (algo == FIRST_FIT) {
        while (current) {
            if (current->free && current->size >= size) {
                best = current;
                break;
            }
            current = current->next;
        }
    } else if (algo == NEXT_FIT) {
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
    
    block *curr = (block *)((char *)ptr - HEADER_SIZE);
    curr->free = 1;
    
    // Coalesce with next block if free
    if (curr->next && curr->next->free) {
        curr->size += curr->next->size + HEADER_SIZE;
        curr->next = curr->next->next;
    }
    
    // Coalesce with previous block if free
    block *prev = head;
    while (prev && prev->next != curr) {
        prev = prev->next;
    }
    if (prev && prev->free) {
        prev->size += curr->size + HEADER_SIZE;
        prev->next = curr->next;
    }
    
    return 0;
}

void umemdump() {
    block *curr = head;
    printf("Memory Dump:\n");
    while (curr) {
        printf("%p: %zu bytes (%s)\n", (void *)((char *)curr + HEADER_SIZE), curr->size - HEADER_SIZE, curr->free ? "free" : "allocated");
        curr = curr->next;
    }
    printf("\n");
}