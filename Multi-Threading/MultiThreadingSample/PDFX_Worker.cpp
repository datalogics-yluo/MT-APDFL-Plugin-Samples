/*
** PDF/x worker
** This thread will convert one document to PDF/x.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  PDFxOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   ConvertOption=[1]                                  Convertor Option selector, 1 to 2. (100 values max!)
**                   RemoveAllAnnotations=[false]                        SettingforPDF/a conversion option "removeAllAnnotations" (100 values max!)
*/
#include "PDFX_Worker.h"
#include "PDFProcessorCalls.h"

ASBool PDFProcessorProgressMonitorCBPDFx (ASInt32 pageNum, ASInt32 totalPages, float current, void *clientData);

    /* Parse the PDF/x conversion thread options into attributes */
void PDFxWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "PDFx";
    worker->LoadPlugins = true;
    worker->paramName = "PDFxOptions";
    worker->type = workerType;

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%AddRedaction.pdf", "Output");

    if (threadAttributes->IsKeyPresent ("RemoveAllAnnotations"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("RemoveAllAnnotations");
        removeAllAnnotationsCount = values->size ();
        for (int index = 0; index < removeAllAnnotationsCount; index++)
            removeAllAnnotations[index] = values->GetValueBool (index);
    }
    else
    {
        removeAllAnnotationsCount = 1;
        removeAllAnnotations[0] = false;
    }

    if (threadAttributes->IsKeyPresent ("ConvertorOption"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("ConvertorOption");
        convertorOptionsCount = values->size ();
        for (int index = 0; index < convertorOptionsCount; index++)
        {
            /* The value should be specified Must be in the table
            ** of defined options value (ConvertorNames).
            ** Of it is not, fail the run!
            */
            convertorOptions[index] = NumberOfPDFXConvertOptions;           /* Set to an invalid value!*/
            char *key = values->value (index);
            for (int x = 0; key[x] != 0; x++)
                key[x] = toupper (key[x]);
            for (int lookup = 0; lookup < NumberOfPDFXConvertOptions; lookup++)
                if (!strcmp (ConvertNames[lookup], values->value (index)))
                {
                    convertorOptions[index] = lookup;
                    break;
                }
            if (convertorOptions[index] == NumberOfPDFXConvertOptions)
            {
                printf ("The PDF/a convertor option \"%s\"does not exist? \n", key);
                exit (-1);
            }
        }
    }
    else
    {
        convertorOptionsCount = 1;
        convertorOptions[0] = 0;
    }

    if (threadAttributes->IsKeyPresent ("UseProgressMonitor"))
        useProgressMonitor = threadAttributes->GetKeyValueBool ("UseProgressMonitor");
};


/* One thread worker procedure */
void PDFxWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;

    if (!silent)
        fprintf (info->logFile, "PDF/x Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    /* Generate input and output file names */
    char *fullFileName = GetInFileName (sequence);
    char *fullOutputFileName = GetOutFileName (sequence, -1);

    DURING
        /* Open the input document */
        APDFLDoc inDoc (fullFileName, true);

    /* Free the input file names */
    free (fullFileName);

    /* Initialize the HFT for the plugin */
    gPDFProcessorHFT = InitPDFProcessorHFT;

    //initialize PDFProcessor plugin
    if (!PDFProcessorInitialize ())
        /* If the plugin cannot be initilized! */
        info->result = 1;
    else
    {
        /* COnstruct the user params recor for the PDF/x conversion */
        PDFProcessorPDFXConvertParamsRec userParams;

        memset ((char *)&userParams, 0, sizeof (PDFProcessorPDFXConvertParamsRec));
        userParams.size = sizeof (PDFProcessorPDFXConvertParamsRec);
        userParams.abortIfXFAPresent = false;
        userParams.colorCompression = kPDFProcessorColorJpegCompression;
        userParams.grayCompression = kPDFProcessorGrayZipCompression;
        userParams.monoCompression = kPDFProcessorMonoCCITTGroup4Compression;
        removeAllAnnotations[sequence % removeAllAnnotationsCount];
        if (useProgressMonitor)
        {
            userParams.progMon = PDFProcessorProgressMonitorCBPDFx;
            userParams.progMonData = &silent;
        }

        /* Create the ouput file ASPath name */
#if !MAC_ENV	
        ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
        ASPathName destFilePath = APDFLDoc::makePath (fullOutputFileName);
#endif

        /* Release the output file name */
        free (fullOutputFileName);

        /* Perform the conversions */
        PDFProcessorConvertAndSaveToPDFX (inDoc.getPDDoc (), destFilePath, ASGetDefaultFileSys (),
            ConvertOptions[convertorOptions[sequence % convertorOptionsCount]], &userParams);

        /* Release the output path name */
        ASFileSysReleasePath (NULL, destFilePath);

        /* terminate the plugin */
        PDFProcessorTerminate ();
    }
    HANDLER
        info->result = 2;
    END_HANDLER


        if (!silent)
            fprintf (info->logFile, "PDF/x Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

}

ASBool PDFProcessorProgressMonitorCBPDFx (ASInt32 pageNum, ASInt32 totalPages, float current, void *clientData)
{
    bool *silent = (bool *)clientData;
    if (!*silent)
        printf ("PDF/x Page %d of %d. Overall Progress = %f %%. \n",
        pageNum + 1, /* Adding 1, since Page numbers are 0-indexed*/
        totalPages,
        current /* Current Overall Progress */);

    //Return 1 to Cancel conversion
    return 0;
}