/* This is a a dummy memory allocator, that will cause APDFL to use malloc internally
**
** Memroy management is accomplished through the APDFL interface element
* TKAllocatorProcs. This structure identiies methods to be used for
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

TKAllocatorProcs *nomemory_access ()
{
    return (NULL);
}
