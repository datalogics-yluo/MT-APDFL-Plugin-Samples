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
** To keep any usage information at all would require a mutex to protect updates to that information!
** I do not want to introduce a mutex for that here. So I iwllkeep no information.
*/
#ifndef MALLOC_MEMORY_h
#define MALLOC_MEMORY_h
#include "PDFInit.h"

TKAllocatorProcs *malloc_access ();

#endif
