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
*/
#include "Flattener_Worker.h"
#include "Utilities.h"
#include "PDFlattenerCalls.h"
#include "ASExtraCalls.h"

ASBool flattenerProgMon (ASInt32 pageNum, ASInt32 totalPages, float current, ASInt32 reserved, void *clientData);
typedef struct flattenerData
{
    bool     silent;
    ASInt32  prevPage;
    FILE    *logFile;
} FlattenerData;


/* Parse the Flattener thread options into attributes
**  UseProgressMonitor is either true or false.
*/
void FlattenWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    WorkerIDEntry = worker;
    worker->name = "Flattener";
    worker->LoadPlugins = false;
    worker->paramName = "FlattenerOptions";

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%AddRedaction.pdf", "Output");

    if (threadAttributes->IsKeyPresent ("UseProgressMonitor"))
        useProgressMonitor = threadAttributes->GetKeyValueBool ("UseProgressMonitor");
};

void FlattenWorker::WorkerThread (ThreadInfo *info)
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
    gPDFlattenerHFT = InitPDFlattenerHFT;;

    //initialize PDFProcessor plugin
    if (!PDFlattenerInitialize ())
        /* If the plugin cannot be initilized! */
        info->result = 1;
    else
    {
        /* COnstruct the user params recor for the flattening */
        PDFlattenerUserParamsRec flattenParams;

        memset (&flattenParams, 0, sizeof (PDFlattenerUserParamsRec));
        flattenParams.size = sizeof (PDFlattenerUserParamsRec);
        //A profiled color space to use for transparent objects. For CMYK, use "U.S. Web Coated (SWOP)v2".
        flattenParams.profileDesc = ASTextFromUnicode ((ASUTF16Val*)"sRGB IEC61966-2.1", kUTF8);
        //The ZIP compression scheme (Flate encoding) for images.
        flattenParams.colorCompression = kPDFlattenerZipCompression;
        //Raster/Vector balance. Use 0.00f for no vectors.
        flattenParams.transQuality = 100.0f;
        // Callback options.

        FlattenerData flattenerInterfaceData;
        flattenerInterfaceData.silent = silent;
        flattenerInterfaceData.logFile = info->logFile;
        flattenerInterfaceData.prevPage = -1;
        
        //Progress monitor callback data. I'm using this data to store the previous page 
        //   the Flattener was working on, using -1 as "hasn't begun yet".
        flattenParams.progressClientData = (void*)&flattenerInterfaceData;

        //The progress monitor callback function.
        if (useProgressMonitor)
            flattenParams.flattenProgress = flattenerProgMon;
        // Tile flattening options

        PDFlattenRec flattener;
        memset (&flattener, 0, sizeof (PDFlattenRec));
        flattener.size = sizeof (PDFlattenRec);

        flattener.tilingMode = kPDNoTiling;
        flattener.tileSizePts = 0;

        //Resolution for flattening the interior of an atomic region.
        flattener.internalDPI = 800.0f;
        //Resolution for flattening edges of atomic regions.
        flattener.externalDPI = 200.0f;

        flattener.clipComplexRegions = false;
        //If we convert stroked elements to filled elements.
        flattener.strokeToFill = true;
        //If we use rastered text instead of native text.
        flattener.useTextOutlines = false;
        //If we attempt to preserve overprint
        flattener.preserveOverprint = true;

        flattener.allowShadingOutput = true;
        flattener.allowLevel3ShadingOutput = true;

        //Maximum image size while flattening. 0 is default.
        flattener.maxFltnrImageSize = 0;
        //Adaptive flattening threshold. Doesn't matter, since we're not doing adaptive tiling. See tilingMode.
        flattener.adaptiveThreshold = 0;

        flattenParams.flattenParams = &flattener;

        /* Perform the flatten document */
        ASUns32 numFlattened = 0;
        ASInt32 flattenResult = PDFlattenerConvertEx2 (inDoc.getPDDoc (),      //The document whose pages we wish to flatten.
            0,                     //The first page to flatten.
            PDDocGetNumPages (inDoc.getPDDoc ()) - 1,   //The last page to flatten.
            &numFlattened,         //PDFlattener sets this to the number of pages
            // it flattened. It will not flatten pages that do not contain transparent elements.
            &flattenParams);             //Flattener options.
        if (flattenResult)
        {
            std::cout << "Flattened " << numFlattened << " pages." << std::endl;
        }
        else
        {
            std::cout << "Flattening failed." << std::endl;
            ASRaise (GenError (genErrGeneral));
        }
        printf ("outputfile name: %s\n", fullOutputFileName);
        /* Save the output PDF Document */
        inDoc.saveDoc (fullOutputFileName); //fullOutputFileName
        /* Release the output file name */
        free (fullOutputFileName);
        ASTextDestroy (flattenParams.profileDesc);
        /* terminate the plugin */
        PDFlattenerTerminate ();
    }
    HANDLER
        info->result = 2;
    END_HANDLER

        if (!silent)
            fprintf (info->logFile, "PDF/a Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}

ASBool flattenerProgMon (ASInt32 pageNum, ASInt32 totalPages, float current, ASInt32 reserved, void *clientData)
{
    //The previous page we were working on.
    FlattenerData *data = (FlattenerData*)clientData;

    //If we've begun a new page, or if we've finished.
    if (pageNum != (data->prevPage) || current == 100.0f)
    {
        //Print the completion percentage.
        fprintf (data->logFile, "[%0.06g%%] Flattening page %01d of %01d.\n", current, pageNum + 1, totalPages);

        //Update previous page.
        data->prevPage = pageNum;
    }

    //Return 1 to cancel Flattening.
    return 0;
}
