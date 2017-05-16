// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//=====================================================================
// Sample: MultiSampleThreading - Demonstrates the use of the APDFL 
//                                library in amulti-threaded environment
//
// Note:
// This sample will extract text from two seperate PDF documents. It
// will save output in the working directory as a .pdf and .txt file. 
// This program demonstrates the APDFL's ability to handle ASCII and
// unicode text extraction.
//
//Steps:
// 1) Initialize the PDWordFinder class and the information we 
//    need to draw the text to the output.
// 2) Iterate through each word of the input document and draw
//    each new line of text to the output document.
// 3) Extract Unicode from a second PDF document.
//=====================================================================



#include "InitializeLibrary.h"
#include "MTHeader.h"
#include "APDFLDoc.h"

#include "DLExtrasCalls.h"
#include "PSFCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "PDPageDrawM.h"
#include "DLExtrasCalls.h"

/* There will be an expandable collection of thread worker method classes that may be run. 
** Each class will have an initialization method, that will set it's unique variables, from a command line array.
** Each class will have a "worker" method that will be executed for it's process.
**
** The command line controls processing
**  "TotalThreads=" gives the total number of threads to run. Default is 100 threads.
**  "ActiveThreads=" gives the number of threads to be run at a given time. Default is 5.
**  "BaseInit=" may be true or false. If true, the library isinitialized inthe base threads. Default is true;
**  "Processes=" is a command seperator list, enclosed in brackets, naming each process to run, in the order they 
**    are to be run. There may be only one, or there may be many. A given process name can be included in the list 
** .  more than once. Threads will be started in the order given here, repeating as the list is exhausted.
**
**  Each type of process may have a set of options specified for it. These may be common, or unique to the process. 
**    These are specified as a common seperated series of keyword/value pairs, inside a set of brackets. The names should
**    be in the form "ProcessNameOptions". Currently, the default options are:
**       "InFileName="  The name of a file used for input in the process. If the file does not exist, the run will fail!
**       "OutFilePath="  The name of an directory tohold output. If there directory does not exist, it will be created.
**                       if the path to the directory does not exist, it willnot be created, and the run will fail!
**       "Silent="  may be true of false. If true, the process will write no messages to the display. Default is true.
*/

/* This is the current set of known workers 
** To this list, add the class name here,
**               Add a field to the union WorkerClassPtr
**               Add a value to the enumertor (These must be a solid set of numbers)
**               Add an entry to the Workers table, giving a textual name to the class,
**                  and a name for it's command line options, 
**               Add a new class derived from workerclass. This class must have a ParseOptions method, 
**                  and a WorkerThread method.
**               Create an initialize an instance of the worker class inthe main procedure.
**               Add a class to the switch in OuterWorker()
*/
class NonAPDFLWorker;
class PDFaWorker ;
class PDFxWorker;
class XPS2PDFWorker;
class TextextWorker;
class RasterizerWorker;

typedef union workerclassptr
{
    NonAPDFLWorker      *NonAPDFL;
    PDFaWorker          *PDFa;
    PDFxWorker          *PDFx;
    XPS2PDFWorker       *XPS2PDF;
    TextextWorker       *TextExtract;
    RasterizerWorker    *Rasterizer;
} WorkerClassPtr;


typedef enum
{
    NONAPDFL,
    PDFA,
    PDFX,
    XPS2PDF,
    TextExtract,
    Rasterizer
};

typedef struct worktypes
{
    char                *name;
    char                *paramName;
    int                  sequence;

} WorkerType;


WorkerType workers[] = {
/*      Name             Options             Id    */
    {"NonAPDFL",    "NonAPDFLOptions",       NONAPDFL },
    {"PDFa",        "PDFaOptions",           PDFA },
    {"PDFx",        "PDFxOptions",           PDFX },
    {"XPS2PDF",     "XPS2PDFOptions",        XPS2PDF },
    {"TextExtract", "TextExtractOptions",    TextExtract },
    { "Rasterizer", "RasterizerOptions",     Rasterizer }
};
#define NumberOfWorkers (sizeof (workers)/sizeof (WorkerType))

WorkerClassPtr workerClasses[NumberOfWorkers];


/* These are the worker class objects that define the process to be 
** executed in a given thread
**
** The set can be expanded indefinately.
*/


/*
** NonAPDFL Worker
** This worker type will simply perform some I/O and some CPU intensive work. It is intended to validate the 
** framework itself, and to provide a baseline against other workers. 
**
** If "NoAPDFL=true", the thread will not even init/term the library. If false, it will Init/Term the library, and give us a picture of behaviour
** doing only init term. Setting "NoAPDFL=true, Reptitions=0" will do nothing EXCEPT init/term the libraries!
**
**  NonAPDFLOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   Repetitions=[5]                                    How many time shall we do the inner loop (100 values max!)
**                   Primes=[1000]                                      How many prime numbers shall we find in CPU loading (100 values max!)
*/
class NonAPDFLWorker : public workerclass
{
    /* This thread will copy a specified file N times, into new specified files.
    ** Each time the file is copied, it will also find the fist 1000 prime numbers!
    */
public:
    NonAPDFLWorker () { Repetitions[0] = 0; RepetitionsCount = 0; Primes[0] = 0; PrimesCount = 0; };
    ~NonAPDFLWorker () { };

    /* Parse the non-APDFL conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions (values, "%AddRedaction.pdf", "Output");

        if (threadAttributes->IsKeyPresent ("Repetitions"))
        {
            valuelist *values = threadAttributes->GetKeyValue ("Repetitions");
            RepetitionsCount = values->size ();
            for (int index = 0; index < RepetitionsCount; index++)
                Repetitions[index] = values->GetValueInt (index);
        }
        else
        {
            RepetitionsCount = 1;
            Repetitions[0] = 5;
        }

        if (threadAttributes->IsKeyPresent ("Primes"))
        {
            valuelist *values = threadAttributes->GetKeyValue ("Primes");
            PrimesCount = values->size ();
            for (int index = 0; index < PrimesCount; index++)
                Primes[index] = values->GetValueInt (index);
        }
        else
        {
            PrimesCount = 1;
            Primes[0] = 1000;
        }

    };

    int FindPrimes (ASUns32 *primes, ASUns32 limit)
    {
        ASUns32 primeCount = 3;
        primes[0] = 1;
        primes[1] = 2;
        primes[2] = 3;
        for (ASUns32 isItPrime = 4; primeCount < limit; isItPrime++)
        {
            ASBool notPrime = false;
            for (ASUns32 lastPrime = 1; lastPrime < primeCount; lastPrime++)
            {
                if (isItPrime % primes[lastPrime] == 0)
                {
                    notPrime = true;
                    break;
                }
            }
            if (notPrime)
                continue;
            primes[primeCount] = isItPrime;
            primeCount++;
        }
        return (primeCount);
    }


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("Non APDFL Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence+1, info->threadNumber + 1);

        /* Presume we willcomplete cleanly */
        info->result = 0;

        /* This process will open an input file, read it's contents to memory, close the file, and write it N times into
        ** new files in the output directory
        */
        char *fullFileName = GetInFileName (sequence);

        /* Open the file, and get it's size */
        FILE *input = fopen (fullFileName, "rb");

        /* free the file name */
        free (fullFileName);

        if (!input)
            /* if we could not open the file, mark as failed or reason 1 */
            info->result = 1;
        else
        {
            fseek (input, 0, SEEK_END);
            size_t fileSize = ftell (input);
            fseek (input, 0, SEEK_SET);

            /* Read the files content into a memory buffer */
            char *buffer = (char *)malloc (fileSize);
            size_t bytesRead = fread (buffer, 1, fileSize, input);

            /* Close the file */
            fclose (input);

            if (bytesRead != fileSize)
            {
                /* If we could not read the entire file, mark as failed for reason 2*/
                info->result = 2;
                free (buffer);
            }
            else
            {
                /* Create n new copies of the file */
                for (int x = 0; x < Repetitions[sequence % RepetitionsCount]; x++)
                {
                    /* Build file name from options */
                    fullFileName = GetOutFileName (sequence, x);

                    /* Open the output file */
                    FILE *output = fopen (fullFileName, "wb");

                    /* Free the file name */
                    free (fullFileName);

                    if (!output)
                        /* If we could not open the output, fail for reason 3 */
                        info->result = 3;
                    else
                    {
                        size_t writeBytes = fwrite (buffer, 1, fileSize, output);
                        if (writeBytes != fileSize)
                            /* if we could not write the entire file, fail for reason 4 */
                            info->result = 4;
                        fclose (output);
                    }

                    /* Burn some CPU as well */
                    ASUns32 *primes = (ASUns32 *)malloc (sizeof (ASUns32) * Primes[sequence % PrimesCount]);
                    ASUns32 primesFound = FindPrimes (primes, Primes[sequence % PrimesCount]);
                }
            }
            free (buffer);
        }

        if (!silent)
            printf ("Non APDFL Worker Thread completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

private:
    ASInt32     Repetitions[100];
    ASInt32     RepetitionsCount;
    ASInt32     Primes[100];
    ASInt32     PrimesCount;
};

/*
** PDF/a worker
** This thread willconvertone document to PDF/a.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  PDFaOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   ConvertOption=[1]                                  Convertor Option selector, 1 to 4. (100 values max!)
**                   RasterizeFontErrors=[false]                        SettingforPDF/a conversion option "rasterizeFontErrors" (100 values max!)
**                   RemoveAllAnnotations=[false]                        SettingforPDF/a conversion option "removeAllAnnotations" (100 values max!)
*/
#include "PDFProcessorCalls.h"
class PDFaWorker : public workerclass
{
#define NumberOfPDFAConvertOptions 4

public:
    PDFaWorker () {
                    ConvertOptions[0] = kPDFProcessorConvertToPDFA1aRGB;
                    ConvertOptions[1] = kPDFProcessorConvertToPDFA1aCMYK;
                    ConvertOptions[2] = kPDFProcessorConvertToPDFA1bRGB;
                    ConvertOptions[3] = kPDFProcessorConvertToPDFA1bCMYK;
                  };

    ~PDFaWorker ()
    { };

    /* Parse the PDF/a conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions (values, "%AddRedaction.pdf", "Output");

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
                /* The value should be specified as 1 to 4 
                ** but must be here converted to 0 to 3
                */
                convertorOptions[index] = values->GetValueInt (index);
                convertorOptions[index] -= 1;
                convertorOptions[index] %= 4;
            }
        }
        else
        {
            convertorOptionsCount = 1;
            convertorOptions[0] = 0;
        }

    };

    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("PDF/a Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

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
                /* COnstruct the user params recor for the PDF/A conversion */
                PDFProcessorPDFAConvertParamsRec userParams;

                memset ((char *)&userParams, 0, sizeof (PDFProcessorPDFAConvertParamsRec));
                userParams.size = sizeof (PDFProcessorPDFAConvertParamsRec);
                userParams.abortIfXFAPresent = false;
                userParams.colorCompression = kPDFProcessorColorJpegCompression;
                userParams.grayCompression = kPDFProcessorGrayZipCompression;
                userParams.monoCompression = kPDFProcessorMonoCCITTGroup4Compression;
                userParams.noRasterizationOnFontErrors = rasterizeFontErrors[sequence % rasterizeFontErrorsCount]; 
                userParams.removeAllAnnotations = removeAllAnnotations[sequence % removeAllAnnotationsCount];


                /* Create the ouput file ASPath name */
#if !MAC_ENV	
                ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
                ASPathName destFilePath = GetMacPath (fullOutputFileName);
#endif

                /* Release the output file name */
                free (fullOutputFileName);
                
                /* Perform the conversions */
                PDFProcessorConvertAndSaveToPDFA (inDoc.getPDDoc (), destFilePath, ASGetDefaultFileSys (), 
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
            printf ("PDF/a Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

private:
    bool rasterizeFontErrors[100];
    int  rasterizeFontErrorsCount;
    bool removeAllAnnotations[100];
    int  removeAllAnnotationsCount;
    ASUns32 convertorOptions[100];
    int  convertorOptionsCount;

    PDFProcessorPDFAConversionOption ConvertOptions[4];
};


/*
** PDF/x worker
** This thread willconvert one document to PDF/x.
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
class PDFxWorker : public workerclass
{
public:
    PDFxWorker () {
                        ConvertOptions[0] = kPDFProcessorConvertToPDFX1a2001;
                        ConvertOptions[1] = kPDFProcessorConvertToPDFX32003;
                  };
    ~PDFxWorker () { };

    /* Parse the PDF/x conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions (values, "%AddRedaction.pdf", "Output");

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
                /* The value should be specified as 1 to 2
                ** but must be here converted to 0 to 1
                */
                convertorOptions[index] = values->GetValueInt (index);
                convertorOptions[index] -= 1;
                convertorOptions[index] %= 2;
            }
        }
        else
        {
            convertorOptionsCount = 1;
            convertorOptions[0] = 0;
        }
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();

        if (!silent)
            printf ("PDF/x Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

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

            /* Create the ouput file ASPath name */
#if !MAC_ENV	
                ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
                ASPathName destFilePath = GetMacPath (fullOutputFileName);
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
            printf ("PDF/x Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    }

private:

    bool removeAllAnnotations[100];
    int  removeAllAnnotationsCount;

    ASUns32 convertorOptions[100];
    int  convertorOptionsCount;

    PDFProcessorPDFXConversionOption ConvertOptions[2];
};

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
#include "XPS2PDFCalls.h"
class XPS2PDFWorker : public workerclass
{
public:
    XPS2PDFWorker () { };
    ~XPS2PDFWorker () { };

    /* Parse the PDFtoXPS conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions(values, "%XPStoPDF.xps", "Output");
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("XPS to PDF Worker Thread! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

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
                ASText jobNameText = ASTextFromUnicode ((ASUTF16Val*)"../../../../Resources/joboptions/Standard.joboptions", kUTF8);
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

        if (!silent)
            printf ("XPS to PDF Worker Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }


};

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
class TextextWorker : public workerclass
{
public:
    TextextWorker () { };
    ~TextextWorker () { };

    /* Parse the text extract conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions (values, "%constitution.pdf", "Output");

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


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("Text Extraction Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

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

            /* Figure out what the first page to do is
            ** (Kind of tricky, when each runmay doa differnt number of pages)
            **/
            ASUns32 position = sequence % pagesCount;
            ASUns64 numberOfPagesDone = 0;
            for (int index = 0; index < sequence; index++)
                numberOfPagesDone += pages[index % pagesCount];

            ASUns32 numberOfPagesPerCycle = 0;
            for (ASUns32 index = 0; index < pagesCount; index++)
                numberOfPagesPerCycle += pages[index];

            ASUns32 firstPageToDo = numberOfPagesDone %= numberOfPagesPerCycle;
            firstPageToDo %= pagesInDocument;

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
            printf ("Text Extraction Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

    private:
        ASUns32 pages[100];
        ASUns32 pagesCount;

};



/*
** Rasterizer Worker
** This worker type will open a document, rasterize one or more pages, one or more times, Optoiopnally placiong the 
** results into a new PDF file. 
**
** If "NoAPDFL=true" must not be specified for this worker type!
**
**  RasterizerOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\constitution.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   NumberOfPages=[1]                                  How many pages shall we rasterize (100 values max!)
**                   Repetitions=[1]                                    How many times shall we rasterize each page done (100 values max!)
**                   SaveImages=false                                   When true, we will save the inages in a PDf document, when false, we will not
**                   Resolution=300                                     Resolution to render image too.
**                   ColorModel={RGB,CMYK,GRAY,DeviceN]                 Which color model to use. RGB is the default.
*/
#include "PagePDECntCalls.h"
class RasterizerWorker : public workerclass
{
    /* This thread will copy a specified file N times, into new specified files.
    ** Each time the file is copied, it will also find the fist 1000 prime numbers!
    */
public:
    RasterizerWorker () { };
    ~RasterizerWorker () { };

    /* Parse the non-APDFL conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        workerclass::ParseOptions (values, "%AddRedaction.pdf", "Output");

        if (threadAttributes->IsKeyPresent ("Repetitions"))
        {
            valuelist *values = threadAttributes->GetKeyValue ("Repetitions");
            RepetitionsCount = values->size ();
            for (int index = 0; index < RepetitionsCount; index++)
                Repetitions[index] = values->GetValueInt (index);
        }
        else
        {
            RepetitionsCount = 1;
            Repetitions[0] = 5;
        }

        if (threadAttributes->IsKeyPresent ("NumberOfPages"))
        {
            valuelist *values = threadAttributes->GetKeyValue ("NumberOfPages");
            pagesCount = values->size ();
            for (ASInt32 index = 0; index < pagesCount; index++)
                pages[index] = values->GetValueInt (index);
        }
        else
        {
            pagesCount = 1;
            pages[0] = 1;
        }

        if (threadAttributes->IsKeyPresent ("SaveImages"))
            saveImages = threadAttributes->GetKeyValueBool ("SaveImages");
        else
            saveImages = false;

        if (threadAttributes->IsKeyPresent ("Resolution"))
            Resolution = threadAttributes->GetKeyValueDouble ("Resolution");
        else
            Resolution = 300.0;

        if (threadAttributes->IsKeyPresent ("ColorModel"))
        {
            valuelist *values = threadAttributes->GetKeyValue ("ColorModel");
            char *name = values->value (0);
            for (int i = 0;name[i] != 0; i++)
                name[i] = toupper (name[i]);
            if (!strcmp (name, "DEVICEN"))
            {
                colorModel = ASAtomFromString ("DeviceN");
                colorComponents = 4;
            }
            else
            {
                if (!strcmp (name, "DEVICEGRAY"))
                {
                    colorModel = ASAtomFromString ("DeviceGray");
                    colorComponents = 1;
                }
                else
                {
                    if (!strcmp (name, "DEVICECMYK"))
                    {
                        colorModel = ASAtomFromString ("DeviceCMYK");
                        colorComponents = 4;
                    }
                    else
                    {
                        colorModel = ASAtomFromString ("DeviceRGB");
                        colorComponents = 3;
                    }
                }
            }
        }
        else
        {
            colorModel = ASAtomFromString ("DeviceRGB");
            colorComponents = 3;
        }

    };

    char *RenderPageToBitmap (PDPage page, ASSize_t *mapSize, ASSize_t *width, ASSize_t *depth)
    {

        /* Get the matrix that transforms user space coordinates to rotated and cropped upright image coordinates.
        **  The origin of this space is the top-left of the rotated, cropped page. Y is decreasing
        */
        ASFixedMatrix matrix;
        PDPageGetFlippedMatrix (page, &matrix);

        //Gets the crop box for a page. The crop box is the region of the page to display and print
        ASFixedRect pageRect;
        PDPageGetCropBox (page, &pageRect);

        /* Set the scale matrix that will be concatenated to the user space matrix
        ** to accomplish the desired resolution 
        */
        ASFixedMatrix scaleMatrix = { FloatToASFixed (Resolution / 72.0), 0, 0, FloatToASFixed (Resolution / 72.0), 0, 0 };

        //Apply the scale to the default matrix
        ASFixedMatrixConcat (&matrix, &scaleMatrix, &matrix);

        /* Apply scale to the destination rectangle*/
        ASFixedRect destRect;
        ASFixedMatrixTransformRect (&destRect, &scaleMatrix, &pageRect);

        /* Generate width and depth */

        /* Construct the draw params record */
        PDPageDrawMParamsRec drawParams;
        memset ((char *)&drawParams, 0, sizeof (PDPageDrawMParamsRec));
        drawParams.size = sizeof (PDPageDrawMParamsRec);
        drawParams.destRect = &destRect;
        drawParams.matrix = &matrix;
        drawParams.csAtom = colorModel;
        drawParams.bpc = 8;
        drawParams.flags = kPDPageDoLazyErase;

        /* Call draw to memory to get buffer size */
        size_t bufferSize = PDPageDrawContentsToMemoryWithParams (page, &drawParams);

        /* If we are doing deviceN, pickup the number of inks here */
        if (drawParams.deviceNColorCount)
            colorComponents = drawParams.deviceNColorCount[0];

        /* If bufferSize is zero, then we set up inks inthe previous call (DeviceN)
        ** So call again to get buffer size
        */
        if (bufferSize == 0)
            bufferSize = PDPageDrawContentsToMemoryWithParams (page, &drawParams);

        /* if buffer size is still zero, something is wrong. Error out!
        */
        if (bufferSize == 0)
            return (NULL);

        /* Allocate a buffer to hold the bitmap */
        char *buffer = (char *)malloc (bufferSize);

        /* Point to it int he draw params.*/
        drawParams.buffer = buffer;
        drawParams.bufferSize = bufferSize;

        /* Draw the page !*/
        PDPageDrawContentsToMemoryWithParams (page, &drawParams);

        /* Tell the caller the size of the map, width, and depth */
        *mapSize = bufferSize;
        *width = abs (ASFixedRoundToInt16 (destRect.right) - ASFixedRoundToInt16 (destRect.left));
        *depth = abs (ASFixedRoundToInt16 (destRect.top) - ASFixedRoundToInt16 (destRect.bottom));

        /* Return the map to the caller */
        return (buffer);
    }

    void AddImageToDoc (PDDoc doc, size_t mapSize, char *map, size_t width, size_t depth, ThreadInfo *info)
    { 
        /* Set upimage Attributes.
        ** Always an XObject image
        */
        PDEImageAttrs attrs;
        memset ((char *)&attrs, 0, sizeof (PDEImageAttrs));
        attrs.flags = kPDEImageExternal;
        attrs.width = width;
        attrs.height = depth;
        attrs.bitsPerComponent = 8;

        /* Always compress image flate */
        PDEFilterArray filterArray;
        memset (&filterArray, 0, sizeof (PDEFilterArray));
        filterArray.numFilters = 1;
        filterArray.spec[0].name = ASAtomFromString ("FlateDecode");

        /* Image is created passed to a 32 bit boundary per row.
        ** For image usage, it must be padded to 8 bits per row
        */
        ASUns32 rowWidthPacked = width * colorComponents;
        ASUns32 rowWidthPadded = (((width * colorComponents * 8) + 31) / 32) * 4;

        /* Repack rows, as needed */
        if (rowWidthPacked != rowWidthPadded)
        {
            /* Remove the row padding bytes */
            for (ASUns16 row = 1; row < depth; row++)
                memmove (&map[row * rowWidthPacked], &map[row * rowWidthPadded], rowWidthPacked);
        }

        /* New size */
        size_t length = width * colorComponents * depth;

        /* Image is erect, and sized 1 point per pixel */
        ASFixedMatrix matrix = { width * fixedOne, 0, 0, depth * fixedOne, 0, 0 };

        /* Color space as per creation
        ** (NOTE: This will not work for deviceN color spaces, will have to 
        **  add logic to create a color space from ink table is we want to support
        ** deviceN colors. Too much to do for now.
        */
        PDEColorSpace cs = PDEColorSpaceCreateFromName (colorModel);

        /* Create a PDE Image (One that can be added to a PDF page */
        PDEImage image = PDEImageCreate (&attrs, sizeof (attrs), &matrix, 0, cs, NULL, &filterArray, NULL, (ASUns8*)map, length);

        /* Create a page to hold the image 
        ** Just the size of the image.
        */
        ASFixedRect pageSize = { 0, depth * fixedOne, width * fixedOne, 0 };
        PDPage page = PDDocCreatePage (doc, PDDocGetNumPages (doc)-1, pageSize);

        /* Get the PDE Content */
        PDEContent content = PDPageAcquirePDEContent (page, 0);

        /* Set the image into the page */
        PDEContentAddElem (content, kPDEAfterLast, (PDEElement)image);

        /* Bind he content back into the page */
        PDPageSetPDEContent (page, 0);

        /* Release resources */
        PDERelease ((PDEObject)image);
        PDPageReleasePDEContent (page, 0);
        PDERelease ((PDEObject)cs);

    }




    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("Rasterizer Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

        /* Generate input name */
        char *fullFileName = GetInFileName (sequence);


        DURING
            /* Open the input document */
            APDFLDoc inDoc (fullFileName, true);
            PDDoc pdDoc = inDoc.getPDDoc ();


            /* Get the number of pages */
            size_t pagesInDocument = PDDocGetNumPages (pdDoc);

            /* Figure out what the first page to do is
            ** (Kind of tricky, when each runmay doa differnt number of pages)
            **/
            ASUns32 position = sequence % pagesCount;
            ASUns64 numberOfPagesDone = 0;
            for (int index = 0; index < sequence; index++)
                numberOfPagesDone += pages[index % pagesCount];

            ASUns32 numberOfPagesPerCycle = 0;
            for (ASUns32 index = 0; index < pagesCount; index++)
                numberOfPagesPerCycle += pages[index];

            ASUns32 firstPageToDo = numberOfPagesDone %= numberOfPagesPerCycle;
            firstPageToDo %= pagesInDocument;

            ASUns32 numberOfPagesToDo = pages[sequence % pagesCount];

            PDDoc outDoc;
            if (saveImages)
                outDoc = PDDocCreate ();
            else
                outDoc = NULL;

            /* Do the pages fromfirst to last, N times */
            for (ASInt32 loop = 0; loop < Repetitions[sequence % RepetitionsCount]; loop++)
            {
                /* Do the requires set of pages */
                for (ASUns32 indexPage = 0; indexPage < pages[sequence % pagesCount]; indexPage++)
                {
                    /* Obtain the current page */
                    ASUns32 pageToDo = (firstPageToDo + indexPage) % pagesInDocument;
                    PDPage page = PDDocAcquirePage (inDoc.getPDDoc(), pageToDo);

                    /* Render the current page */
                    ASSize_t mapsize, width, depth;
                    char *mapBuffer = RenderPageToBitmap (page, &mapsize, &width, &depth);

                    /* Release the current page */
                    PDPageRelease (page);

                    /* If we are saving the images, make the map and image, and 
                    ** add it to the images document
                    */
                    if (saveImages)
                    {
                        AddImageToDoc (outDoc, mapsize, mapBuffer, width, depth, info);
                    }

                    /* Free the bitmap */
                    free (mapBuffer);

                }
            }

            if (saveImages)
            {
                /* The automatic logic will use he same suffix for the output as the input, so change the suffix here */
                char *fullOutputFileName = GetOutFileName (sequence, -1);
#if !MAC_ENV	
                ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
                ASPathName destFilePath = GetMacPath (fullOutputFileName);
#endif

                PDDocSave (outDoc, PDSaveFull | PDSaveCollectGarbage, destFilePath, NULL, NULL, NULL);
                ASFileSysReleasePath (NULL, destFilePath);
                free (fullOutputFileName);
                PDDocClose (outDoc);
            }

        HANDLER
            info->result = 1;
        END_HANDLER

        if (!silent)
            printf ("Rasterizer Worker Thread completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

private:
    ASInt32     Repetitions[100];
    ASInt32     RepetitionsCount;
    ASInt32     pages[100];
    ASInt32     pagesCount;
    ASBool      saveImages;
    double      Resolution;
    ASAtom      colorModel;
    ASInt8      colorComponents;
};


/* This procedure is the one called by all threads!
**  it uses the workerclass object to create the library, and 
** collect startup information, then call the WorkerThread of the 
** approrpriate worker object to handle the bulk of the processing. 
** finally, it uses the workerclass to close the library, and collect 
** timing information.
*/
void outerWorker (ThreadInfo *info)
{
    workerclass *baseObject = (workerclass *)(info->object);
   

    baseObject->startThreadWorker (info);
    switch (info->objectType)
    {
    case NONAPDFL:
        ((NonAPDFLWorker *)baseObject)->WorkerThread (info);
        break;
    case PDFA:
        ((PDFaWorker *)baseObject)->WorkerThread (info);
        break;
    case PDFX:
        ((PDFxWorker *)baseObject)->WorkerThread (info);
        break;
    case XPS2PDF:
        ((XPS2PDFWorker *)baseObject)->WorkerThread (info);
        break;
    case TextExtract:
        ((TextextWorker *)baseObject)->WorkerThread (info);
        break;
    case Rasterizer:
        ((RasterizerWorker *)baseObject)->WorkerThread (info);
        break;
    default:
         baseObject->WorkerThread (info);
        break;
    }

    baseObject->endThreadWorker (info);
}


/* program takes the following command line attributes:
**      TotalThreads (Default to 100)       How many threads total whall we run
**      ActiveThreads (Default to 8)        Maximum threads started at any time
**      BaseInit (Defaults to false)        If true, init/term library on base thread. If false, do not.
**
**      Each worker thread may also define a command line attribute. It is in the form:
**          threadtype=[Attribute=value attribute=value ...]. 
**      It will be parsed into a dictionary in the thread, and used as attributes to control that particular thread.
**
** if the argument count is 2, and argument 1 is the name of a file, which can be opened and read, 
** argument 2 will be presumed to be a command file, in the form of the command line options, and will be opened, 
** read, and interpreted.
**
**/

int main(int argc, char** argv)
{
    int errCode = 0;                /* return code tho the user */

    /* Parse the comand line */
    attributes SampleAttributes = attributes (argc, argv);


    /* If we are using a base thread library, start it now */
    APDFLib *baseInstance = NULL;
    if (SampleAttributes.GetKeyValueBool ("BaseInit"))
        baseInstance = new APDFLib ();

    /* Construct the array of worker types 
    ** Set establish the options for each type*/
    workerClasses[NONAPDFL].NonAPDFL = new NonAPDFLWorker ();
    workerClasses[NONAPDFL].NonAPDFL->ParseOptions (SampleAttributes.GetKeyValue (workers[NONAPDFL].paramName));


    workerClasses[PDFA].PDFa = new PDFaWorker ();
    workerClasses[PDFA].PDFa->ParseOptions (SampleAttributes.GetKeyValue (workers[PDFA].paramName));

    workerClasses[PDFX].PDFx = new PDFxWorker ();
    workerClasses[PDFX].PDFx->ParseOptions (SampleAttributes.GetKeyValue (workers[PDFX].paramName));

    workerClasses[XPS2PDF].XPS2PDF = new XPS2PDFWorker ();
    workerClasses[XPS2PDF].XPS2PDF->ParseOptions (SampleAttributes.GetKeyValue (workers[XPS2PDF].paramName));

    workerClasses[TextExtract].TextExtract = new TextextWorker ();
    workerClasses[TextExtract].TextExtract->ParseOptions (SampleAttributes.GetKeyValue (workers[TextExtract].paramName));

    workerClasses[Rasterizer].TextExtract = new TextextWorker ();
    workerClasses[Rasterizer].TextExtract->ParseOptions (SampleAttributes.GetKeyValue (workers[Rasterizer].paramName));


    /* construct an array of threads to be run */
    int totalThreads = 100;
    if (SampleAttributes.IsKeyPresent ("TotalThreads"))
        totalThreads = SampleAttributes.GetKeyValueInt ("TotalThreads");

    int activeThreads = 5;
    if (SampleAttributes.IsKeyPresent ("ActiveThreads"))
        activeThreads = SampleAttributes.GetKeyValueInt ("ActiveThreads");

    /* This will be the list of threads to run */
    ThreadInfo *threads = (ThreadInfo *)malloc (sizeof (ThreadInfo) * totalThreads);

    /* We will alternate threads through the list of processes defined */
    int processes = 1;
    if (SampleAttributes.IsKeyPresent ("Processes"))
        processes = SampleAttributes.GetKeyValue ("Processes")->size ();

    WorkerClassPtr *workerList = (WorkerClassPtr*)malloc (processes * sizeof (WorkerClassPtr));
    int            *workerTypeList = (int *)malloc (processes * sizeof (int));
    workerList[0].PDFa = workerClasses[PDFA].PDFa;
    workerTypeList[0] = PDFA;

    if (SampleAttributes.IsKeyPresent ("Processes"))
    {
        valuelist *list = SampleAttributes.GetKeyValue ("Processes");
        for (int index = 0; index < processes; index++)
        {
            workerList[index].PDFa = NULL;
            char *processName = list->value (index);
            for (int y = 0; processName[y] != 0; y++)
                processName[y] = toupper (processName[y]);
            for (int x = 0; x < NumberOfWorkers; x++)
            {
                char workerName[1024];
                strcpy (workerName, workers[x].name);
                for (int y = 0; workerName[y] != 0; y++)
                    workerName[y] = toupper (workerName[y]);

                if (!strcmp (workerName, processName))
                {
                    workerList[index].PDFa = workerClasses[workers[x].sequence].PDFa;
                    workerTypeList[index] = workers[x].sequence;
                    break;
                }
            }
            if (workerList[index].PDFa == NULL)
            {
                sprintf ("There is no worker type \"%s\".\n", processName);
                exit (-1);
            }
        }
    }

    /* Now, "threads" contains a threadinfo structure for each thread we want to run, 
    ** and "workerList" contains a list of the workers we want to run, in the order we 
    ** want to run them. Populate these into the "threads" list, so each thread will know what 
    ** worker to use.
    */
    int type = 0;
    for (int index = 0; index < totalThreads; index++)
    {
        if (type >= processes)
            type = 0;
        memset ((char *)&threads[index], 0, sizeof (ThreadInfo));
        threads[index].threadNumber = index;
        threads[index].object = (void *)workerList[type].PDFa;
        threads[index].objectType = workerTypeList[type];
        type++;
    }

    /* The "threads" table is now populated with the type or worker to run. We just need to 
    ** start "actualCount" threads, and each time a thread ends, start a new thread
    */
    int runningThreads = 0;             /* Number of threads currently active */
    int completedThreads = 0;           /* Number of threads currently completed */
    int startedThreads = 0;             /* Number of threads so far started */

    /* This is used by wait for many to wait for the next thread to finish */
    SDKThreadID *activeThreadArray = (SDKThreadID *)malloc (sizeof (SDKThreadID) * activeThreads);

    /* THis is used interna;y to identify the thread that finished 
    ** (So we don't have to search the list, looking for the the matching thread ID)
    */
    ThreadInfo **activeThreadInfo = (ThreadInfo **)malloc (sizeof (ThreadInfo *) * activeThreads);

    while (completedThreads < totalThreads)
    {
        /* If we have less threads running than we want active, and we have not 
        ** already started all threads, start a thread!
        */
        if ((startedThreads < totalThreads) && (runningThreads < activeThreads))
        {
            createThread (outerWorker, threads[startedThreads]);
            activeThreadArray[runningThreads] = threads[startedThreads].threadID;
            activeThreadInfo[runningThreads] = &threads[startedThreads];
            startedThreads++;
            runningThreads++;
            continue;
        }

        /* If we get here, we have as many threads running as we can,.
        ** So wait for some to complete!
        */
        if (runningThreads)
        {
            /* Wait for the first in the list of threads to complete
            */
#ifdef WIN_PLATFORM
            ASInt32 index = WaitForAnyThreadComplete (activeThreadArray, runningThreads);
#else
            ASInt32 index = -1;
            while (0)
            {
                for (int x = 0; x < runningThreads; x++)
                {
                    if (activeThreadInfo[x]->threadCompleted)
                    {
                        index = x;
                        break;
                    }
                }
                if (index != -1)
                    break;
                usleep (1000);
            }
#endif

            /* A thread completed! */
            completedThreads++;

            /* If we want to "Do" anything with the thread that just finished, here is where we should 
            ** do it. activeThreadInfo[index] is a pointer to the threads ThreadInfo block
            */
            ThreadInfo *doneThread = activeThreadInfo[index];
#ifdef WIN_PLATFORM
            /* For windows, it is easier to collect thread info after the thread completes 
            ** The values are in FILETIME, which is nano seconds since 1/1/1601 (For some ofd reason), 
            ** They arested in two adjacent 32 bit integers, sequence such tht they can be considered a 
            ** single 64 bit integer 
            */
            FILETIME start, end, kernel, cpuTime;
            ASUns64 *start64 = (ASUns64*)&start, *end64 = (ASUns64 *)&end, *kernel64 = (ASUns64 *)&kernel, *cpu64 = (ASUns64 *)&cpuTime;
            GetThreadTimes (doneThread->threadID, &start, &end, &kernel, &cpuTime);
            end64[0] -= start64[0];
            cpu64[0] += kernel64[0];
            doneThread->wallTimeUsed = ((((end64[0] * 1.0) / 1000) /* nano to micro */ / 1000) /* Micro to milli */ / 1000 /* Milli to seconds*/);
            doneThread->cpuTimeUsed = ((((cpu64[0] * 1.0) / 1000) /* nano to micro */ / 1000) /* Micro to milli */ / 1000 /* Milli to seconds*/);
#endif

            /* If we are not silent, then display a status for the thread completing */
            if (!doneThread->silent)
                printf ("Thread %01d completed in %0.6g seconds wall, %0.10g seconds CPU, with code %01d.\n",
                doneThread->threadNumber+1, doneThread->wallTimeUsed, doneThread->cpuTimeUsed, doneThread->result);

            /* end the thread */
            destroyThread (doneThread);

            /* If the thread to finish was NOT the last thread, then shift all of the arrays
            ** up to remove this thread from the array 
            */
            if (index < (runningThreads - 1))
            {
                for (int x = index; x < (runningThreads - 1); x++)
                {
                    activeThreadArray[x] = activeThreadArray[x + 1];
                    activeThreadInfo[x] = activeThreadInfo[x + 1];
                }
            }

            /* One less running thread */
            runningThreads--;

            continue;
        }

        /* We should never get here. Something went wrong in our counts!*/
        printf ("Something went wrong in our threading counts?\n We say we started %01d of %01d threads,"
                " completed %&01d, but have no threads active?\n", startedThreads, totalThreads, completedThreads);
        exit (-2);
    }


    /* Shut down the working thread objects */
    for (int index = 0; index < NumberOfWorkers; index++)
        delete (workerClasses[index].NonAPDFL);

    /* If we are using a base thread library, stop it now */
    if (SampleAttributes.GetKeyValueBool ("BaseInit"))
        delete baseInstance;

    return errCode;
};
