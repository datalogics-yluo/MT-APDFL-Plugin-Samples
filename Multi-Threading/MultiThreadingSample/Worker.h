/* Define the base worker class. 
** This will do handling of options and actions common to all 
** worker thread types.
*/
#ifndef WORKER_H
#define WORKER_H

#include "MtHeader.h"
#include "ASExpt.h"
#include <time.h>

/* This enumeration is used to identify worker by class within the set of all classes
** based on workerclass. As new classes are added, an enumeration for them should be 
** added as well. 
** The last member of the enumeration must always be "NumberOfWorkers"
*/
typedef enum
{
    NONAPDFL,
    PDFA,
    PDFX,
    XPS2PDF,
    TextExtract,
    Rasterizer,
    Flattener,
    NumberOfWorkers
} EnumOfWorkers;



class workerclass;

/* Thread Communication */
typedef struct
{
    ASInt32         threadNumber;                       /* Serial number of thread in set of threads */
    ASUns32         sequence;
    SDKThreadID     threadID;                           /* Platform dependent thread "handle" */
    void           *object;                             /* Worker Thread Object */
    APDFLib        *instance;                           /* APDFL Library instance */
    ASInt32         result;                             /* Numeric result, unique to worker type. But "zero" is always "No Problem" */
    time_t          startTime, endTime;                 /* Used in Unix only, wall time started/stopped */
    clock_t         startCPU, endCPU;                   /* Used in Unix only to track CPU time used. */
    double          wallTimeUsed, cpuTimeUsed;          /* Walltime start to finish, and CPU time (user and kernal) consumed */
    double          percentUtilized;                    /* Percentage of CPU time in wall time */
    bool            silent;                             /* When true, write nothing to stdout! */
    bool            noAPDFL;                            /* When true, do not init/term the library in this thread! */
    bool            threadCompleted;                    /* Mark the thread complete for unix to locate it */
    FILE           *logFile;                            /* Write status message to this file */
    bool            logFileSet;                         /* If log file is set, then default "silent" to "false". */
    bool            LoadPlugins;                        /* If true, we must load plugins for this type of worker. */
    bool            UseTempMemFileSys;                  /* If true, use the Ram File Sys for temp files. */
} ThreadInfo;

/* Worker Type Communication */
typedef struct worktypes
{
    char                *name;
    char                *paramName;
    int                  sequence;
    bool                 LoadPlugins;
} WorkerType;

/* This is a base class for the worker threads
*/
class workerclass
{
public:

    WorkerType      *WorkerIDEntry;     /* Entry in the worker table of this specific worker */
    EnumOfWorkers    workerType;

    /* Every worker thread needs an input and output file
    ** The ideal worker has a sequence of them, so that we may
    ** eiher select all workers to be identical, or force them to behave differently,
    ** and reveal diferent conflicts.
    ** These will be set in the standard options logic (workerclass::ParserOptions()) from the command
    ** line keyword "InFileName".
    */
    int          InFileCount;
    char       **InFilePath;
    char       **InFileName;
    char       **InFileSuffix;

    /* Some classes (Ergo: merge documents) may have more than one input file
    ** The following two options are established to allow for 2 or three input
    ** files
    */
    int          InFile2Count;
    char       **InFile2Path;
    char       **InFile2Name;
    char       **InFile2Suffix;
    int          InFile3Count;
    char       **InFile3Path;
    char       **InFile3Name;
    char       **InFile3Suffix;

    /* I don't ever see a real reason for seperate output paths,  But leave this for Symmetry
    ** These will be set in the standard options logic (workerclass::ParserOptions()) from the command
    ** line keyword "OutFilePath".
    */
    int          OutPathCount;
    char       **OutFilePath;

    /* When true (default) do not print messages from this object
    ** This will be set in the standard options logic (workerclass::ParserOptions()) from the command
    ** line keyword "Silent".*/
    bool        silent;

    /* When true (Default false) do not initiate/terminate the library per thread!
    ** (This may be set to true by default for a given worker class, overridding both
    **  the setting from the options, and the default)
    ** This will be set in the standard options logic (workerclass::ParserOptions()) from the command
    ** line keyword "noAPDFL".
    */
    bool        noAPDFL;

    /* Dictionary of options for this object */
    attributes *threadAttributes;

    workerclass ();
    ~workerclass ();

    /* return the worker type */
    EnumOfWorkers GetWorkerClass () { return workerType; }

    /* We should never use the base class worker thread */
    void WorkerThread (ThreadInfo *) { return; }

    /* Get the full file name a sequence in the file name list */
    char *GetInFileName (int threadSequence);
    char *GetInFile2Name (int threadSequence);
    char *GetInFile3Name (int threadSequence);

    /* Get an output file name for a sequence in the file and 
    ** output directory list. This name will always be in the form
    ** outputDirectoryPath[n] pathSep InputFileName[n].InputFileSuffix[n]
    */
    char * GetOutFileName (int threadSequence, int inner = -1);

    /* Processing done at the start of every worker thread! */
    void startThreadWorker (ThreadInfo *info);

    /* Processing done at the end of every worker thread! */
    void endThreadWorker (ThreadInfo *info);

    /* Utiltity to split a file name into path, name, suffix */
    void splitpath (char *path, char **toPath, char **filename, char **suffix);

    /* Utility to append the standard data path to a file name, if the 
    ** Name begins with a %.
    */
    char *expandFileName (char *inname);

    /* Parse a file name in to the first input file array */
    void parseOneFileName (int index, char *inname);

    /* Parse a file name in to the second input file array */
    void parseTwoFileName (int index, char *inname);

    /* Parse a file name in to the third input file array */
    void parseThreeFileName (int index, char *inname);

    /* Obtain the worker thread specific options from the Frame Attributes dictionary, 
    ** and obtain options that apply to all types of threads. 
    */
    void    ParseOptions (attributes *FrameAttributes, char *defaultInFileName, char *defaultOutFilePath);
};

#endif
