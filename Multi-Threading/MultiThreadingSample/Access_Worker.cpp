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
#include "Access_Worker.h"
#include "PDCalls.h"
#include "PERCalls.h"
#include "PagePDECntCalls.h"

    /* Parse the Access thread options into attributes */
void AccessWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "Access";
    worker->LoadPlugins = true;
    worker->paramName = "AccessOptions";
    worker->type = workerType;

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%AddRedaction.pdf", "Output");

};


/* One thread worker procedure */
void AccessWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;

    if (!silent)
        fprintf (info->logFile, "Access Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    /* Generate input file name */
    char *fullFileName = GetInFileName (sequence);

    DURING
        /* Open the input document */
        PDDoc inDoc = OpenSampleFile (fullFileName);

        /* Free the input file names */
        free (fullFileName);

        /* Get the page count */
        size_t pageCount = PDDocGetNumPages (inDoc);

        /* Foreach page in the document */
        for (size_t index = 0; index < pageCount; index++)
        {
            /* Acauire the page */
            PDPage page = PDDocAcquirePage (inDoc, (ASInt32)index);

            /* Acquire it's PDEContent */
            PDEContent content = PDPageAcquirePDEContent (page, 0);

            /* Get the number ofentries in the PDEContent */
            size_t numElems = PDEContentGetNumElems (content);

            /* Release the PDE Content */
            PDPageReleasePDEContent (page, 0);

            /* Release the Page */
            PDPageRelease (page);
        }

        /* Close the document */
        PDDocClose (inDoc);

    HANDLER
        info->result = 1;
    END_HANDLER


        if (!silent)
            fprintf (info->logFile, "Access Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

}

