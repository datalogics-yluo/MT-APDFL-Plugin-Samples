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
#include "tcmalloc_memory.h"

static TKAllocatorProcs    tcmalloc_access_block;

#if 0           /* I cannot yet find a way to link this dynamically? */

void *tcmalloc_allocate (void * cleintData, size_t size)
{
    return (tcmalloc (size));
}

void *tcmalloc_reallocate (void * cleintData, void *pointer, size_t size)
{
    return (tcrealloc (pointer, size));
}

void tcmalloc_free (void * cleintData, void *ptr)
{
    tcfree (ptr);
    return;
}
size_t tcmalloc_remaining (void * cleintData)
{
    return (1024 * 1024 * 1024 * 1);
}

TKAllocatorProcs *tcmalloc_access ()
{
    tcmalloc_access_block.allocProc = tcmalloc_allocate;
    tcmalloc_access_block.reallocProc = tcmalloc_reallocate;
    tcmalloc_access_block.freeProc = tcmalloc_free;
    tcmalloc_access_block.memAvailProc = tcmalloc_remaining;
    tcmalloc_access_block.clientData = NULL;
    return &tcmalloc_access_block;
}
#else
TKAllocatorProcs *tcmalloc_access ()
{
    return NULL;
}
#endif
