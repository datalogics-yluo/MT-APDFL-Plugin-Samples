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
#ifndef RPMALLOC_MEMORY_h
#define RPMALLOC_MEMORY_h

#ifndef MAC_ENV

#include "PDFInit.h"
#include "rpmalloc.h"

TKAllocatorProcs *rpmalloc_access ();       /* Call this method to acquire an APDFL memory Manager Interface */

/* Call this interface once, from the main line, before any apdfl libraries are 
** started.this call establishes the overall functionality of the memory manager
*/
void rpmalloc_master_initialize ();

/* Call this interface once, from the mainline, after all APDFL libraries are 
** terminated. This call shuts down the memory manager completly.
*/
void rpmalloc_master_finalize ();

/* Call this interface before APDFL initialization for each library initialized. */
void rpmalloc_init ();

/* Call this interface after APDFL termination, for each library terminated. */
void rpmalloc_finalize ();



#endif
#endif
