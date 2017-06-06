/* This is a basic APDFL memory manager built using system malloc
**
** Memroy management is accomplished through the APDFL interfce element
* TKAllocatorProcs. This structure identiies methods to be used for 
** allocating, reallocating, and freeing memory. It also contains a
** reference to a method that may indicate how much memory is avaialable
** to be allcoated. Generally, that last method is not accurately set.
** When it indicates a lower amount, it triggers cache cleanup.
**
** Allocators used in the MT Framework should be fairly minimal. 
** Keep a current and highwater count only. Force alignment to 
** 64 bit bounds.
*/
#ifndef NO_MEMORY_H
#define NO_MEMORY_H
#include "PDFInit.h"

TKAllocatorProcs *nomemory_access ();

#endif
