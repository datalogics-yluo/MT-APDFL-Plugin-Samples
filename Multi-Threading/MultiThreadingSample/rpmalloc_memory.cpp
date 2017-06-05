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
#include "rpmalloc_memory.h"
#include "PDFInit.h"

static TKAllocatorProcs    rpmalloc_access_block;


void *rpmalloc_allocate (void * cleintData, size_t size)
{
    return (rpmalloc (size));
}

void *rpmalloc_reallocate (void * cleintData, void *pointer, size_t size)
{
    return (rprealloc (pointer, size));
}

void rpmalloc_free (void * cleintData, void *ptr)
{
    rpfree (ptr);
    return;
}
size_t rpmalloc_remaining (void * cleintData)
{
    return (1024 * 1024 * 1024 * 1);
}

TKAllocatorProcs *rpmalloc_access ()
{
    rpmalloc_access_block.allocProc = rpmalloc_allocate;
    rpmalloc_access_block.reallocProc = rpmalloc_reallocate;
    rpmalloc_access_block.freeProc = rpmalloc_free;
    rpmalloc_access_block.memAvailProc = rpmalloc_remaining;
    rpmalloc_access_block.clientData = NULL;
    return &rpmalloc_access_block;
}

