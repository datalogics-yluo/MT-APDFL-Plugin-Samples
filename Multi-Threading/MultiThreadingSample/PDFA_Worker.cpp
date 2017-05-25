/*
** PDF/a worker
** This thread will convert one document to PDF/a.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  PDFaOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   ConvertOption=[RGB1A]                              Convertor Option selector, [RGB1A | RGB1B | CMYK1A | CMYK1B] (100 values max!)
**                   RasterizeFontErrors=[false]                        SettingforPDF/a conversion option "rasterizeFontErrors" (100 values max!)
**                   RemoveAllAnnotations=[false]                        SettingforPDF/a conversion option "removeAllAnnotations" (100 values max!)
**                   UseProgressMonitor=false                           Uses or does not use the progress monitor. Singular Value! Default is off.

*/
#include "PDFA_Worker.h"
#include "PDFProcessorCalls.h"

ASBool PDFProcessorProgressMonitorCBPDFa (ASInt32 pageNum, ASInt32 totalPages, float current, void *clientData);

/* Parse the PDF/a conversion thread options into attributes
**  RasterizeFontErrors is true/false, and is passed into the PDF/a convertor
**  RemoveAllAnnotations is true/false, and is passed into the PDF/a convertor
**  ConvertOption is one of RGB1a, RGB1b, CMYK1a, CMYK1b.
*/
void PDFaWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "PDFa";
    worker->LoadPlugins = false;
    worker->paramName = "PDFaOptions";

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%AddRedaction.pdf", "Output");

    /* RasteriseFontErrors will be passed to the PDF/a processor
    */
    if (threadAttributes->IsKeyPresent ("RasterizeFontErrors"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("RasterizeFontErrors");
        rasterizeFontErrorsCount = values->size ();
        for (int index = 0; index < rasterizeFontErrorsCount; index++)
            rasterizeFontErrors[index] = values->GetValueBool (index);
    }
    else
    {
        rasterizeFontErrorsCount = 1;
        rasterizeFontErrors[0] = false;
    }

    /* RemoveAllAnnotations will be passed to the PDF/a processor*/
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

    /* ConvertOptions will be converted to a numeric selector, 
    ** passed into the PDF/a convertor, to control which type of conversion
    ** is to be done.
    */
    if (threadAttributes->IsKeyPresent ("ConvertOption"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("ConvertOption");
        convertorOptionsCount = values->size ();
        for (int index = 0; index < convertorOptionsCount; index++)
        {
            /* The value should be specified Must be in the table 
            ** of defined options value (ConvertorNames).
            ** Of it is not, fail the run!
            */
            convertorOptions[index] = NumberOfPDFAConvertOptions;           /* Set to an invalid value!*/
            char *key = values->value (index);
            for (int x = 0; key[x] != 0; x++)
                key[x] = toupper (key[x]);
            for (int lookup = 0; lookup < NumberOfPDFAConvertOptions; lookup++)
                if (!strcmp (ConvertorNames[lookup], values->value (index)))
                {
                    convertorOptions[index] = lookup;
                    break;
                }
            if (convertorOptions[index] == NumberOfPDFAConvertOptions)
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
void PDFaWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;

    if (!silent)
        fprintf (info->logFile, "PDF/a Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

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
        /* COnstruct the user params record for the PDF/A conversion */
        PDFProcessorPDFAConvertParamsRec userParams;

        memset ((char *)&userParams, 0, sizeof (PDFProcessorPDFAConvertParamsRec));
        userParams.size = sizeof (PDFProcessorPDFAConvertParamsRec);
        if (useProgressMonitor)
        {
            userParams.progMon = PDFProcessorProgressMonitorCBPDFa;
            userParams.progMonData = &silent;
        }
        userParams.colorCompression = kPDFProcessorColorJpegCompression;
        userParams.noRasterizationOnFontErrors = false;
        userParams.removeAllAnnotations = false;


        /* Create the ouput file ASPath name */
#if !MAC_ENV	
        ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
        ASPathName destFilePath = APDFLDoc::makePath (fullOutputFileName);
#endif

        /* Release the output file name */
        free (fullOutputFileName);

        /* Perform the conversions */
        PDFProcessorConvertAndSaveToPDFA (inDoc.getPDDoc (), destFilePath, ASGetDefaultFileSys (),
            ConvertOptions[convertorOptions[sequence % convertorOptionsCount] + 1], &userParams);

        /* Release the output path name */
        ASFileSysReleasePath (NULL, destFilePath);

        /* terminate the plugin */
        PDFProcessorTerminate ();
    }
    HANDLER
        info->result = 2;
    END_HANDLER

        if (!silent)
            fprintf (info->logFile, "PDF/a Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}


ASBool PDFProcessorProgressMonitorCBPDFa (ASInt32 pageNum, ASInt32 totalPages, float current, void *clientData)
{
    bool *silent = (bool *)clientData;
    if (!*silent)
        printf ("PDF/a Page %d of %d. Overall Progress = %f %%. \n",
        pageNum + 1, /* Adding 1, since Page numbers are 0-indexed*/
        totalPages,
        current /* Current Overall Progress */);

    //Return 1 to Cancel conversion
    return 0;
}
