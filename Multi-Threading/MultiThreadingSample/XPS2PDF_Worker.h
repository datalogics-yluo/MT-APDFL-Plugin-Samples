/*
** XPS2PDF worker
** This thread will convert one document from XPS to PDF.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  XPS2PDFOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\XPStoPDF.xps]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
*/
#include "MTHeader.h"
#include "Worker.h"

class XPS2PDFWorker : public workerclass
{
public:
    XPS2PDFWorker () 
    { 
        workerType = XPS2PDF;
    };
    ~XPS2PDFWorker () { };

    /* Parse the XPS2PDf  thread options into attributes
    **  XPS2PDf has no special options
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

};