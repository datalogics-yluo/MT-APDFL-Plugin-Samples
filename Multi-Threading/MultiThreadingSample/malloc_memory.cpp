/* This is a basic APDFL memory manager built using system malloc
**
** Memory management is accomplished through the APDFL interface element
* TKAllocatorProcs. This structure identies methods to be used for
** allocating, reallocating, and freeing memory. It also contains a
** reference to a method that may indicate how much memory is avaialable
** to be allcoated. Generally, that last method is not accurately set.
** When it indicates a lower amount, it triggers cache cleanup.
**
** Allocators used in the MT Framework should be fairly minimal.
** Keep a current and highwater count only. Force alignment to
** 64 bit bounds.
**
** I can update usages and amounts without a mutex, because this object is created
** specific to the thread that is using it!.
*/

#include <stdlib.h>
#include "malloc_memory.h"

static TKAllocatorProcs    malloc_access_block;


void *malloc_allocate (void * cleintData, size_t size)
{
    return (malloc (size));
}

void *malloc_reallocate (void * cleintData, void *pointer, size_t size)
{
    return (realloc (pointer, size));
}

void malloc_free (void * cleintData, void *ptr)
{
    free (ptr);
    return;
}
size_t malloc_remaining (void * cleintData)
{
    return (1024 * 1024 * 1024 * 1);
}

TKAllocatorProcs *malloc_access ()
{
    malloc_access_block.allocProc = malloc_allocate;
    malloc_access_block.reallocProc = malloc_reallocate;
    malloc_access_block.freeProc = malloc_free;
    malloc_access_block.memAvailProc = malloc_remaining;
    malloc_access_block.clientData = NULL;
    return &malloc_access_block;
}

