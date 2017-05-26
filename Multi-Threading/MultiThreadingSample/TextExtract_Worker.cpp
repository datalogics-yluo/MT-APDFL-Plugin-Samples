/*
** TextExtract  worker
** This thread extract the content of N pages (Specified in page count, below)
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  PDFxOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\constitution.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   NumberOfPages=[1]                                  Number of pages of content to extract in this thread (100 values max!)
*/
#include "TextExtract_Worker.h"
#include "Utilities.h"
#include "PDCalls.h"

/* Parse the TextExtract  thread options into attributes
**  Number Of Pages is the number of pages to render in one thead. It may be a list.
*/
void TextextWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "TextExtract";
    worker->LoadPlugins = false;
    worker->paramName = "TextExtractOptions";

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%constitution.pdf", "Output");

    if (threadAttributes->IsKeyPresent ("NumberOfPages"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("NumberOfPages");
        pagesCount = values->size ();
        for (ASUns32 index = 0; index < pagesCount; index++)
            pages[index] = values->GetValueInt (index);
    }
    else
    {
        pagesCount = 1;
        pages[0] = 1;
    }

};


void TextextWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;
    if (!silent)
        fprintf (info->logFile, "Text Extraction Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    /* Generate input name */
    char *fullFileName = GetInFileName (sequence);


    DURING
        /* Open the input document */
        APDFLDoc inDoc (fullFileName, true);
    PDDoc pdDoc = inDoc.getPDDoc ();


    /* Get the number of pages */
    size_t pagesInDocument = PDDocGetNumPages (pdDoc);

    /* */
    PDWordFinderConfigRec wfConfig;
    memset (&wfConfig, 0, sizeof (PDWordFinderConfigRec));
    wfConfig.recSize = sizeof (PDWordFinderConfigRec);

    /* Create a word finder */
    PDWordFinder wordFinder = PDDocCreateWordFinderEx (pdDoc, WF_LATEST_VERSION, false, &wfConfig);

    /* How many pages have we already done*/
    ASUns64 numberOfPagesDone = 0;
    for (int index = 0; index < sequence; index++)
        numberOfPagesDone += pages[index % pagesCount];

    /* reduce modulos the number of pages in the document */
    ASUns32 firstPageToDo = numberOfPagesDone % pagesInDocument;

    ASUns32 numberOfPagesToDo = pages[sequence % pagesCount];

    for (ASUns32 indexPage = 0; indexPage < pages[sequence % pagesCount]; indexPage++)
    {

        /* Acquire the words for the Nth page */
        PDWord wordList;
        ASInt32 numWordsFound;
        ASUns32 pageToDo = (firstPageToDo + indexPage) % pagesInDocument;

        PDWordFinderAcquireWordList (wordFinder, pageToDo, &wordList, NULL, NULL, &numWordsFound);

        /* The automatic logic will use he same suffix for the output as the input, so change the suffix here */
        char *fullOutputFileName = GetOutFileName (sequence, pageToDo + 1);
        char *suffix = &fullOutputFileName[strlen (fullOutputFileName) - 3];
        suffix[0] = 0;
        strcat (fullOutputFileName, "txt");

        /* Create a file to hold the words */
        FILE *wordFile = fopen (fullOutputFileName, "w");

        /* Release the file name */
        free (fullOutputFileName);

        /* Write document and page name to result file */
        fprintf (wordFile, "%s Page: %01d", fullFileName, pageToDo);


        for (int i = 0; i < numWordsFound; ++i)
        {
            /* Get the next word */
            PDWord nextWord = PDWordFinderGetNthWord (wordFinder, i);

            char workWord[1024];
            PDWordGetString (nextWord, workWord, 1024);
            fprintf (wordFile, "(%3d)  %s\n", i, workWord);
        }

        /* Close the word file */
        fclose (wordFile);

        /* Release the word finder results */
        PDWordFinderReleaseWordList (wordFinder, 0);
    }

    /* Release the word finder itself*/
    PDWordFinderDestroy (wordFinder);

    /* free input file name  */
    free (fullFileName);

    HANDLER
        info->result = 1;
    END_HANDLER

        if (!silent)
            fprintf (info->logFile, "Text Extraction Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}
