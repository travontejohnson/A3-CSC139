#include <stdio.h>
#include "umem.h"

int main() {
    printf("Initializing memory allocator with 1MB using BEST_FIT\n");
    umeminit(1024 * 1024, BEST_FIT);
    
    printf("Allocating 10 blocks of various sizes\n");
    void *ptrs[10];
    ptrs[0] = umalloc(1);
    ptrs[1] = umalloc(16);
    ptrs[2] = umalloc(32);
    ptrs[3] = umalloc(64);
    ptrs[4] = umalloc(128);
    ptrs[5] = umalloc(256);
    ptrs[6] = umalloc(512);
    ptrs[7] = umalloc(1024);
    ptrs[8] = umalloc(2048);
    ptrs[9] = umalloc(4096);
    
    printf("Memory state after allocations:\n");
    umemdump();
    
    printf("Freeing blocks 0, 3, 5, 7, 9\n");
    ufree(ptrs[0]);
    ufree(ptrs[3]); 
    ufree(ptrs[5]);
    ufree(ptrs[7]);
    ufree(ptrs[9]);
    
    printf("Memory state after frees:\n");
    umemdump();
    
    printf("Allocating a 500 byte block\n");
    void *ptr = umalloc(500);
    
    printf("Memory state after 500 byte allocation:\n");
    umemdump();
    
    ufree(ptr);
    
    printf("Testing WORST_FIT allocation\n");
    umeminit(1024 * 1024, WORST_FIT);
    ptrs[0] = umalloc(1);
    ptrs[1] = umalloc(16);
    ptrs[2] = umalloc(32);
    umemdump();
    
    return 0;
}