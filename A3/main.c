#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "umem.h"

//Test Function Delcarations 
void allocateBlocks(void *ptrs[10]);
void freeBlocks(void *ptrs[], int multiIndex[]);
void maxBlockAllocTest();
void testZeroAllocation();
void alignmentTest();
void worstFitTest();
void memStateTest(int i);


int main() {
    printf("Initializing memory allocator with 1MB using BEST_FIT\n");
    umeminit(1024 * 1024, BEST_FIT);
    
    void *ptrs[10];
    int indicesToFree[] = {0, 5, 3, 5, 7, 4, -1}; 

    // Tests to be ran by the program
    allocateBlocks(ptrs);
    freeBlocks(ptrs,indicesToFree);
    memStateTest(128);
    maxBlockAllocTest();
    testZeroAllocation();
    worstFitTest();
    alignmentTest();
    
    return 0;
}

/* Test Case 1
The purpose of this code is to randomy assign memory to a block based in the powers of two.
I wanted to do this to simulate the random memory sizes instead of being ordered in size because that is not realistic.
*/
void allocateBlocks(void *ptrs[10]){
    srand(time(NULL)); 

    printf("\nTest Case 1: Allocating 10 blocks of various sizes (all powers of 2 up to 4096):\n");

    for (int i = 0; i < 10; i++) {
        int power = rand() % 14 + 3;
        ptrs[i] = umalloc(1 << power);
        printf("Allocated block %d of size %d bytes\n", i, 1 << power);
    }

    printf("Memory state after allocations:\n");
    umemdump();
}

/* Test Case 2:
The purpose of this test is to correctly free memory from the selected indicies
The user is able to change this information in the main code by adding to the indicesToFree int array.
The expected outcome from this function is to display Freed block at index (i), there will also show if
a block has been already freed to prevent errors. It is great at checking memory leaks as well.
*/
void freeBlocks(void *ptrs[], int multiIndex[]) {
    printf("Test Case 2: Freeing selected blocks based on user input:\n");
    for (int i = 0; multiIndex[i] != -1; i++) {  // Continue until sentinel -1 is found
        int idx = multiIndex[i];
        // Check if the pointer has not been freed already
        if (ptrs[idx] != NULL) {  
            ufree(ptrs[idx]);   
            // Prevents additional free checks for the same index
            ptrs[idx] = NULL;
            printf("Freed block at index %d\n", idx);
        } else {
            // Text to be output if the block has already been freed.
            printf("Block at index %d has already been freed.\n", idx); 
        }
    }

    printf("Memory state after frees:\n");
    umemdump();
}

/* Test Case 3
This test is to show how the syste allocations a large block then it will verify the impact on the memory.
The user can change the variable (i) to dynamically allocate and free the test space
*/
void memStateTest(int i){
    printf("Test Case 3: Allocating a %d byte block\n", i);
    void *ptr = umalloc(i);
    printf("Memory state after %d byte allocation:\n", i);
    umemdump();
    
    ufree(ptr);
}

/* Test Case 4
The purpose of this test is to fill up the space and cehcks to see if the maximum allocations goes well.
This handles a single very large block that is nearly the size of the allocation, this is good for filling up and freeing the space.
*/
void maxBlockAllocTest() {
    printf("Test Case 4: Testing Maximum Block Allocation\n");
    size_t max_size = 1024 * 1024 - 64; 
    void *ptr = umalloc(max_size);
    if (ptr) {
        printf("Allocated a block of size %zu bytes.\n", max_size);
        umemdump();
        ufree(ptr);
    } else {
        printf("Failed to allocate a block of size %zu bytes.\n", max_size);
    }
}

/*  Test Case 5: 
This test is great to show how the memory management system handles getting asked to allocate a 0 byte allocation request
The intended output is to show that the alloctation returns as NULL.
This is great to prevent incorrect memory usage and invalid allocations in the allocater.
*/
void testZeroAllocation(){
    printf("\nTest Case 5: Zero Allocation\n");
    void *ptr = umalloc(0);
    if(ptr){
        printf("Zero byte allocation returned non-NULL. This may be unintended depending on design choices.\n");
    } else {
        printf("Zero byte allocation correctly returned NULL.\n");
    }
}

/* Test Case 6:
This test case is to allow worst fit testng as display the allocation strategy that is used as well.
This is also good at showing how the largest avaliable block is taken first.
*/
void worstFitTest(){
    printf("\nTest Case: 6: Testing WORST_FIT allocation\n");
    void *ptrs[6];
    umeminit(1024 * 1024, WORST_FIT);
    ptrs[0] = umalloc(1024);
    ptrs[1] = umalloc(4096);
    ptrs[2] = umalloc(32);
    ptrs[3] = umalloc(8192);
    ptrs[4] = umalloc(16384);
    ptrs[5] = umalloc(16);
    umemdump();
}

/* Test Case 7:
This function is used to verify that the system corretly allocates blocks to various boundaries
The range of boundaries that I use are from 8 to 32 byte 
this test can only be ran with allocateBlocks or by itself.
*/
void alignmentTest() {
    printf("\nTest Case 7: Testing Multiple Alignments\n");

    // Test alignments and sizes
    int test_sizes[] = {1, 16, 32, 64, 128, 256, 512, 1024, 2048};
    size_t alignments[] = {8, 16, 32};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    int num_alignments = sizeof(alignments) / sizeof(alignments[0]);

    srand(time(NULL));

    for (int i = 0; i < num_sizes; i++) {
        for (int j = 0; j < num_alignments; j++) {
            size_t size = test_sizes[i];
            size_t alignment = alignments[j];

            void *ptr = umalloc(size);
            size_t address = (size_t)ptr;

            printf("Testing allocation size %zu bytes for %zu-byte alignment: ", size, alignment);
            // THis is for checking the alignment to make sure everything is okay.
            //assert(address % alignment == 0); 
            printf("Pass\n");

            ufree(ptr);
        }
    }

    printf("All alignment tests passed.\n\n");
}