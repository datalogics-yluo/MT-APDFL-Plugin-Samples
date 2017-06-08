/*
** Access worker
** This thread will open a PDF document, and obtian and release the PDEContet of each page.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  AccessOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   ConvertOption=[1]                                  Convertor Option selector, 1 to 2. (100 values max!)
**                   RemoveAllAnnotations=[false]                        SettingforPDF/a conversion option "removeAllAnnotations" (100 values max!)
*/
#ifndef ACCESS_WORKER_H
#define ACCESS_WORKER_H

#include "Worker.h"
#include "PDFProcessorCalls.h"
#include "CosCalls.h"
#include "ASCalls.h"
#include "Utilities.h"
class AccessWorker : public workerclass
{
#define NumberOfPDFXConvertOptions 2
public:
    AccessWorker ()
    {
        workerType = Access;
    };
    ~AccessWorker () { };

    /* Parse the Access conversion thread options into attributes
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

};
#endif
