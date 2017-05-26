/*
** Flattener worker
** This thread will flatten document.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  FlattenOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   UsePregressMonitor=false                           When true, use a progress monitor, else, do not.
*/
#include "Worker.h"


class FlattenWorker : public workerclass
{
public:
    FlattenWorker ()
    {
        workerType = Flattener;
    };

    ~FlattenWorker ()
    { };

    /* Parse the Flattener thread options into attributes
    **  UseProgressMonitor is either true or false.
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

private:
    bool useProgressMonitor;
};
