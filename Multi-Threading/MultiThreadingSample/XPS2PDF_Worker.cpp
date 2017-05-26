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
#include "Utilities.h"
#include "XPS2PDF_Worker.h"
#ifndef MAC_ENV
#include "XPS2PDFCalls.h"
#endif
#include "CosCalls.h"
#include "ASCalls.h"
#include "ASExtraCalls.h"

/* Parse the XPS2PDf  thread options into attributes
**  XPS2PDf has no special options
*/
void XPS2PDFWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "XPS2PDf";
    worker->LoadPlugins = false;
    worker->paramName = "XPS2PDFOptions";

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%XPStoPDF.xps", "Output");
};


void XPS2PDFWorker::WorkerThread (ThreadInfo *info)
{
#ifdef MAC_ENV
    int sequence = info->sequence;
    if (!silent)
        fprintf (info->logFile, "XPS to PDF Worker Thread does nothing on the Macintosh! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
#else
    int sequence = info->sequence;
    if (!silent)
        fprintf (info->logFile, "XPS to PDF Worker Thread! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    DURING

        /* Set up the HFT for XPS2PDF */
        gXPS2PDFHFT = InitXPS2PDFHFT;

        /* Load and initialize the XPS2PDF plugin */
        if (!XPS2PDFInitialize ())
            info->result = 1;
        else
        {
            /* Set up the job options. */
            ASCab settings = ASCabNew ();

            //The .joboptions file specifies a great number of settings which determine exactly how the PDF document
            //is created by the converter.
            ASText jobNameText = ASTextFromUnicode ((ASUTF16Val*)"../Resources/joboptions/Standard.joboptions", kUTF8);
            ASCabPutText (settings, "PDFSettings", jobNameText);

            //Specify which description in the .joboptions file we will use.
            //There are many others, for different langauges. See the file.
            ASText language = ASTextFromUnicode ((ASUTF16Val*)"ENU", kUTF8);
            ASCabPutText (settings, "PDFSettingsLang", language);

            /* Generate input and output file names */
            char *fullFileName = GetInFileName (sequence);
            char *fullOutputFileName = GetOutFileName (sequence, -1);

            /* The automatic logic will use he same suffix for the output as the input, so change the suffix here */
            char *suffix = &fullOutputFileName[strlen (fullOutputFileName) - 3];
            suffix[0] = 0;
            strcat (fullOutputFileName, "pdf");

            /* Create the ASPathName for the input (XPS) document */
            ASPathName asInPathName = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullFileName, 0);

            /* release the input file name */
            free (fullFileName);

            /* Convert the document */
            PDDoc outputDoc = NULL;
            int ret_val = XPS2PDFConvert (settings, 0, asInPathName, NULL, &outputDoc, NULL);

            //If we succeeded, XPS2PDFConvert returns 1.
            if (ret_val != 1)
                info->result = 2;
            else
            {
                /* Save the output PDF Document */
                APDFLDoc outAPDoc;
                outAPDoc.pdDoc = outputDoc;
                outAPDoc.saveDoc (fullOutputFileName);
            }

            /* release the output file name */
            free (fullOutputFileName);

            /* Release other resources created */
            ASCabDestroy (settings);
            ASFileSysReleasePath (NULL, asInPathName);

        }

        /* Terminate the plugin */
        XPS2PDFTerminate ();

    HANDLER
        info->result = 3;
    END_HANDLER
#endif

    if (!silent)
        fprintf (info->logFile, "XPS to PDF Worker Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}
