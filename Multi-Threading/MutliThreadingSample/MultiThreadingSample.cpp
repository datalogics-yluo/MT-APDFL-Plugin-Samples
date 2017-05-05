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
*/
class NonAPDFLWorker;
class PDFaWorker ;
class PDFxWorker;
class XPS2PDFWorker;
class TextextWorker;

typedef union workerclassptr
{
    NonAPDFLWorker      *NonAPDFL;
    PDFaWorker          *PDFa;
    PDFxWorker          *PDFx;
    XPS2PDFWorker       *XPS2PDF;
    TextextWorker       *TextExtract;
} WorkerClassPtr;


typedef enum
{
    NONAPDFL,
    PDFA,
    PDFX,
    XPS2PDF,
    TextExtract
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
    {"TextExtract", "TextExtractOptions",    TextExtract }
};
#define NumberOfWorkers (sizeof (workers)/sizeof (WorkerType))

WorkerClassPtr workerClasses[NumberOfWorkers];

/* Each orker class must define a default for it's input file. 
** These are generally in terms of the distribution directories 
** file structure. For that reason, it is easiest to define 
** the path here, as a single, platform specific define
*/
#ifdef WIN_PLATFORM
#define inputPath "..\\..\\..\\..\\Resources\\Sample_Input\\"
#else
#define inputPath "../_Input/"
#endif

/* These are the worker class objects that define the process to be 
** executed in a given thread
**
** The set can be expanded indefinately.
*/

class NonAPDFLWorker : public workerclass
{
    /* This thread will copy a specified file N times, into new specified files.
    */
public:
    NonAPDFLWorker () { Repetitions = 0; };
    ~NonAPDFLWorker () { };

    /* Parse the non-APDFL conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        parseCommonOptions (values, inputPath "AddRedaction.pdf", "Output");

        if (threadAttributes->IsKeyPresent ("Repetitions"))
            Repetitions = threadAttributes->GetKeyValueInt ("Repetitions");
        else
            Repetitions = 5;
    };

    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("Non APDFL Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence+1, info->threadNumber + 1);

        /* Presume we willcomplete cleanly */
        info->result = 0;

        /* This process willopen an input file, read it's contents to memory, close the file, and write it N times into
        ** new files inthe output directory
        */
        char fullFileName[2048];
        char SepChar[2] = { PathSep, 0 };
        sprintf (fullFileName, "%s%s%s.%s", InFilePath, SepChar, InFileName, InFileSuffix);

        /* Open the file, and get it's size */
        FILE *input = fopen (fullFileName, "rb");
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
                for (int x = 0; x < Repetitions; x++)
                {
                    sprintf (fullFileName, "%s%s%s_%01d_%01d.%s", OutFilePath, SepChar, InFileName, sequence + 1, x + 1, InFileSuffix);
                    FILE *output = fopen (fullFileName, "wb");
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
                }
            }
            free (buffer);
        }

        if (!silent)
            printf ("Non APDFL Worker Thread completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

private:
    ASInt32     Repetitions;
};


class PDFaWorker : public workerclass
{
public:
    PDFaWorker () { };
    ~PDFaWorker ()
    { };

    /* Parse the PDF/a conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        parseCommonOptions (values, inputPath "AddRedaction.pdf", "Output");
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("PDF/a Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

        if (!silent)
            printf ("PDF/a Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

private:

};

class PDFxWorker : public workerclass
{
public:
    PDFxWorker () { };
    ~PDFxWorker () { };

    /* Parse the PDF/x conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        parseCommonOptions (values, inputPath "AddRedaction.pdf", "Output");
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();

        if (!silent)
            printf ("PDF/x Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

        if (!silent)
            printf ("PDF/x Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    }

};


class XPS2PDFWorker : public workerclass
{
public:
    XPS2PDFWorker () { };
    ~XPS2PDFWorker () { };

    /* Parse the PDFtoXPS conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        parseCommonOptions (values, inputPath "XPStoPDF.xps", "Output");
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("XPS to PDF Worker Thread! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

        if (!silent)
            printf ("XPS to PDF Worker Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }


};


class TextextWorker : public workerclass
{
public:
    TextextWorker () { };
    ~TextextWorker () { };

    /* Parse the text extract conversion thread options into attributes */
    void ParseOptions (valuelist *values)
    {
        parseCommonOptions (values, inputPath "ExtractText.pdf", "Output");
    };


    /* One thread worker procedure */
    void WorkerThread (ThreadInfo *info)
    {
        int sequence = getNextSequence ();
        if (!silent)
            printf ("Text Extraction Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

        if (!silent)
            printf ("Text Extraction Worker Thread Completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
    }

};

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
**/

int main(int argc, char** argv)
{
    int errCode = 0;                /* return code tho the user */

    /* Parse the comand line */
    attributes SampleAttributes = attributes (argc, argv);


    /* If we are using a base thread library, start it now */
    APDFLib *baseInstance = NULL;
    if (SampleAttributes.GetKeyValueBool ("BaseInit", false))
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
    ThreadInfo *activeThreadInfo[1000]; /* NOTE! This fixed array puts a ceiling on the active thread value! */

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

        /* If we get here, we have as manny threads running as we can,.
        ** So wait for some to complete!
        */
        if (runningThreads)
        {
            /* Wait for the first in the list of threads to complete
            */
            ASInt32 index = WaitForAnyThreadComplete (activeThreadArray, runningThreads);

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
            doneThread->wallTimeUsed = ((((end64[0] * 1.0) / 1000) /* nano to micro */ / 1000) /* Micro to milli */ / 1000 /* Milli to seconds*/);
            doneThread->cpuTimeUsed = ((((cpu64[0] * 1.0) / 1000) /* nano to micro */ / 1000) /* Micro to milli */ / 1000 /* Milli to seconds*/);
#endif

            /* If we are not silent, then display a status for the thread completing */
            if (!doneThread->silent)
                printf ("Thread %01d completed in %0.6g seconds wall, %0.6g seconds CPU, with code %01d.\n",
                doneThread->threadNumber, doneThread->wallTimeUsed, doneThread->cpuTimeUsed, doneThread->result);


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
    if (SampleAttributes.GetKeyValueBool ("BaseInit", false))
        delete baseInstance;

    return errCode;
};
