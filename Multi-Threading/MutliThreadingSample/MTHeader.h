// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//=====================================================================
// This header file defines classs used in the multiThreading sample.

#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <vector>
#include <string>
#include <time.h>

#ifdef WIN_PLATFORM
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "InitializeLibrary.h"

/* These two classes are used to implement a command line parser, for the syntax:
**
**  Command Line = pair { space | pair}*
**  pair = name {=value}
**  name = any text character except space or "=".
**  value = {[} string {"," | spaces | string}* ]} \
**  string = any text character except "," o space
**
** So a command line may look, for instance, like:
**    TotalThreads=100 Threads=10 processes=[PDFX, PDFA,TextExtract]
**
** Keys and Values will always be stored upper cased, and tested upper cased.
**
** When a file name is required, providing the file name, with a leading "%", will 
** replace the % with the "standard" path to the release diretory sample test data.
*/

/* For platform dependent file name construction */
/* Each worker class must define a default for it's input file.
** These are generally in terms of the distribution directories
** file structure. For that reason, it is easiest to define
** the path here, as a single, platform specific define
*/
#ifdef WIN_PLATFORM
#define PathSep '\\'
#define access _access
#define mkdir _mkdir
#define inputPath "..\\..\\..\\..\\Resources\\Sample_Input"
#else
#define PathSep  '/'
#define inputPath "../_Input"
#endif

class valuelist
{
private:
    std::vector<char *> list;
public:
    valuelist (char *string)
    {
        /* trim any spaces off the start of the string */
        char *start = string;
        while (start[0] == ' ')
            start++;

        /* Check is this is a list, or a single entry
        ** (Presence of brackets)
        */
        if (start[0] == '[')
        {
            /* remove the array open bracket */
            start++;

            /* There is a multiple entry list here */
            /* cut off strings until we have them all */
            while ((start[0] != ']') && (start[0] != 0))
            {
                while (start[0] == ' ')
                    start++;
                if ((start[0] != ']') && (start[0] != 0))
                {
                    char *end = strstr (start, ",");
                    if (!end)
                        end = strstr (start, " ");
                    if (!end)
                        end = strstr (start, "]");
                    if (!end)
                        end = start + strlen (start);
                    int length = end - start;
                    char *saveValue = (char *)malloc (length + 1);
                    strncpy (saveValue, start, length);
                    saveValue[length] = 0;
                    list.push_back (saveValue);
                    start = end;
                    if (end[0] == ',')
                        start++;
                }
            }
        }
        else
        {
            /* This is a single entry list */
            char *saveValue = (char *)malloc (strlen (start) + 1);
            strcpy (saveValue, string);
            list.push_back (saveValue);
        }
    }

    ~valuelist ()
    {
        for (unsigned int count = 0; count < list.size (); count++)
            free (list[count]);
    }

    int size () { return list.size (); }

    char *value (int index) { return list[index]; }

    int GetValueInt (unsigned int index)
    {
        if (index >= list.size())
                return 0;
        return (atoi (list[index]));
    }

    bool GetValueBool (unsigned int Index)
    {
        if (Index >= list.size ())
            return false;
        char *value = list[Index];
        for (int i = 0; value[i] != 0; i++)
            value[i] = toupper (value[i]);
        if (!strcmp (value, "TRUE"))
            return true;
        if (!strcmp (value, "FALSE"))
            return false;
        return false;
    }

};


/* Attributes takes a count, and a list of strings
** initially, this is intended as the argument intput to a 
** standard "main", argC and argV.
**
** However, we are using the same construct to parse substrings
** (classIdOptions) as well.
**
** Finally, if the arg count is 2, argument 1 is not null, and 
** argument 2 is the name of a file that can be opened for read access,
** we will presume argument 2 desribes a file containing arguments in the 
** same form as the command line, and us it in place of the command line.
*/
class attributes
{
public:
    typedef std::map<std::string, valuelist *> attributeDict;
    attributeDict keys;

    attributes (int argcIn, char **argvIn)
    {
        int argc = argcIn;
        char **argv = argvIn;
        char *buffer = NULL;

        /*
        ** Special case:
        **
        ** Argument count is 2
        ** Argument 1 is not a null pointer,
        ** Argument 2 is a readable file name
        **   Then read the file, and pare it into the form of command lines, and parse it into
        ** the attributes dictionary
        */
        if ((argc == 2) && (argv[0] != NULL))
        {


            if (!access (argv[1], 04))
            {
                FILE *commandFile = fopen (argv[1], "r");
                fseek (commandFile, 0, SEEK_END);
                size_t fileSize = ftell (commandFile);
                char *buffer = (char *)malloc (fileSize + 10);
                fseek (commandFile, 0, SEEK_SET);
                memset (buffer, 0, fileSize + 10);
                fread (buffer, 1, fileSize, commandFile);
                buffer[fileSize] = 0;
                fclose (commandFile);

#define maxArguments 1000
                char **argumentList = (char **)malloc (sizeof (char *) * maxArguments);
                argumentList[0] = NULL;
                int argumentCount = 1;
                ASUns32 position = 0;
                while (position < fileSize)
                {
                    argumentList[argumentCount] = buffer + position;
                    argumentCount++;
                    while ((buffer[position] > ' ') && (position < fileSize))
                        position++;
                    buffer[position] = 0;
                    position++;
                    while ((buffer[position] <= ' ') && (position < fileSize))
                        position++;
                }
                argc = argumentCount;
                argv = argumentList;
            }
        }

        /* for each command line argument.
        ** NOTE: Command line arguments are delimted by spaces. so is you write a list
        ** value for an argument that includes spaces, put the argument in quotes!
        */
        for (int count = 0; count < argc; count++)
        {
            /* define attributes name and value.*/
            char *key = NULL, *value = NULL;


            if (count == 0)
            {
                /* Attribute zero is always the path of the
                ** executible being run.
                */
                key = (char *)malloc (12);
                strcpy (key, "ProcessPath");
                if (argv[0] == NULL)
                    continue;
                value = (char *)malloc (strlen (argv[0]) + 1);
                strcpy (value, argv[0]);
            }
            else
            {
                /* This can be just a key word, or a keyword and a value */
                char *equal = strstr (argv[count], "=");
                if (equal)
                {
                    /* Sperate out and store the key */
                    int keyLen = equal - argv[count];
                    key = (char *)malloc (keyLen + 1);
                    strncpy (key, argv[count], keyLen);
                    key[keyLen] = 0;

                    /* a value may be a single string, or is may be
                    ** an array of strings. If it is an array, it may have embedded
                    ** spaces. That would break up the string in the command parser.
                    **
                    ** check is the string is an array,and if that array completes
                    ** in this string. If it does not, append the following strings,
                    ** until we find the end of the array
                    */
                    value = (char *)malloc (strlen (&equal[1]) + 1);
                    strcpy (value, &equal[1]);
                    if (value[0] == '[')
                    {
                        int nest = 1;
                        /* To make this simplier, we are going to replace '[' with '}'
                        ** during the nest detection, we will convert themback after we
                        ** complete the string.
                        */
                        value[0] = '{';
                        while (nest)
                        {
                            /* find the end array marker,
                            ** allowing for nesting.
                            */
                            char *endarray = strstr (value, "]");
                            char *nestarray = strstr (&value[1], "[");

                            /* append following groups until we find a close array */
                            if ((!endarray) && ((count + 1) < argc))
                            {
                                value = (char *)realloc (value, strlen (value) + strlen (argv[count + 1]) + 2);
                                strcat (value, " ");
                                strcat (value, argv[count + 1]);
                                count++;
                                continue;
                            }

                            /* if we found an open array before the close array, remove is, and increment the
                            ** nest count by one
                            */
                            if ((nestarray) && (nestarray < endarray))
                            {
                                nest++;
                                nestarray[0] = '{';
                                continue;
                            }

                            /* If we found an end array, decriment the nest count */
                            if ((!nestarray) && (endarray))
                            {
                                nest--;
                                endarray[0] = '}';
                                continue;
                            }

                            if (count == argc - 1)
                            {
                                /* If we reach the end, and there is a group open, just close it.*/
                                nest--;
                                continue;
                            }
                        }

                        /* replace the braces with brackets */
                        char *brace = strstr (value, "{");
                        while (brace)
                        {
                            brace[0] = '[';
                            brace = strstr (value, "{");
                        }
                        brace = strstr (value, "}");
                        while (brace)
                        {
                            brace[0] = ']';
                            brace = strstr (value, "}");
                        }
                    }

                }
                else
                {
                    /* There is no value associated with this keyword */
                    key = (char *)malloc (strlen (argv[count]) + 1);
                    value = (char *)malloc (1);
                    value = NULL;
                    strcpy (key, argv[count]);
                }
            }
            for (int index = 0; key[index] != 0; index++)
                key[index] = toupper (key[index]);
            valuelist *values = new valuelist (value);
            AddKeyValue (key, values);
        }

        /* If we parsed a file, 
        ** free resources here
        */
        if (buffer)
        {
            free (buffer);
            free (argv);
        }
    }

    ~attributes ()
    {
    }

    void AddKeyValue (char *key, valuelist *value)
    {
        std::string newKey(key);

        keys.insert(std::pair<std::string, valuelist*>(newKey, value));

    }

    valuelist *GetKeyValue (char *key)
    {
        char localKey[100];
        strcpy (localKey, key);
        for (int index = 0; localKey[index] != 0; index++)
            localKey[index] = toupper (localKey[index]);
       
        std::string actualKey (localKey);
        attributeDict::iterator found = keys.find (actualKey);
        if (found != keys.end())
        {
            return (found->second);
        }
        else
            return (NULL);
    }

    bool IsKeyPresent (char *key)
    {
        char localKey[100];
        strcpy (localKey, key);
        for (int index = 0; localKey[index] != 0; index++)
            localKey[index] = toupper (localKey[index]);

        std::string actualKey (localKey);
        attributeDict::iterator found = keys.find (actualKey);
        return (found != keys.end());
    }

    bool GetKeyValueBool (char *key)
    {
        if (IsKeyPresent (key))
        {
            valuelist *values = GetKeyValue (key);
            if (values->size () == 0)
                return false;
            char *value = values->value (0);
            for (int index = 0; value[index] != 0; index++)
                value[index] = toupper (value[index]);
            if (!strcmp (value, "TRUE"))
                return true;
            if (!strcmp (value, "FALSE"))
                return false;
            return false;
        }
        else
            return false;
    }

    int GetKeyValueInt (char *key)
    {
        if (IsKeyPresent (key))
        {
            valuelist *values = GetKeyValue (key);
            if (values->size () == 0)
                return 0;
            char *value = values->value (0);
            return (atoi (value));
        }
        else
            return 0;
    }

    double GetKeyValueDouble (char *key)
    {
        if (IsKeyPresent (key))
        {
            valuelist *values = GetKeyValue (key);
            if (values->size () == 0)
                return 0;
            char *value = values->value (0);
            return (atof (value));
        }
        else
            return 0.0;
    }
};






/* Device independent defininitions for 
** Multi Threading thread creation/deletion/Communication and synchronization
*/
#ifdef WIN_PLATFORM
#include "windows.h"
#include "process.h"

typedef HANDLE SDKThreadID;
#define ThreadFuncReturnType unsigned int WINAPI
typedef LPVOID ThreadFuncArgType;
typedef ThreadFuncReturnType ThreadFuncType (ThreadFuncArgType);
#define createThread( func, tinfo ) ((tinfo.threadID = (SDKThreadID)_beginthreadex( \
	NULL, 0, (ThreadFuncType *)(&func), &tinfo, 0, NULL)) != 0)
#define destroyThread( tinfo ) CloseHandle( tinfo->threadID )

#define WaitForAnyThreadComplete(list, size) \
    WaitForMultipleObjects (size, (HANDLE *)list, false, INFINITE) - WAIT_OBJECT_0;

        

typedef CRITICAL_SECTION CSMutex;
#define EnterCS( CSMutex ) EnterCriticalSection( &CSMutex )
#define LeaveCS( CSMutex ) LeaveCriticalSection( &CSMutex )
#define InitCS( CSMutex ) InitializeCriticalSection( &CSMutex )
#define DestroyCS( CSMutex ) DeleteCriticalSection( &CSMutex )

#else
#include <pthread.h>
#include <unistd.h>

typedef pthread_t SDKThreadID;
typedef void * ThreadFuncReturnType;
typedef void * ThreadFuncArgType;
typedef ThreadFuncReturnType ThreadFuncType (ThreadFuncArgType);
#define createThread( func, tinfo ) (pthread_create( &tinfo.threadID, NULL, (ThreadFuncType *)func, &tinfo) == 0)
#define destroyThread( tinfo ) pthread_detach( tinfo->threadID )


typedef pthread_mutex_t *CSMutex;
#define InitCS( CSMutex ) do { \
	CSMutex = (pthread_mutex_t *)malloc( sizeof(pthread_mutex_t) ); \
	pthread_mutex_init( CSMutex, NULL ); \
	} while (0)

#define EnterCS( CSMutex ) pthread_mutex_lock( CSMutex )
#define LeaveCS( CSMutex ) pthread_mutex_unlock( CSMutex )
#define DestroyCS( CSMutex ) do { \
	pthread_mutex_destroy( CSMutex ); \
	free( CSMutex ); \
	} while (0)

#endif


/* Thread Communication */
class workerclass;

typedef struct
{
    ASInt32         threadNumber;                       /* Serial number of thread in set of threads */
    ASUns32         sequence;
    SDKThreadID     threadID;                           /* Platform dependent thread "handle" */
    void           *object;                             /* Worker Thread Object */
    int             objectType;                         /* Enumerator giving thread type */
    APDFLib        *instance;                           /* APDFL Library instance */
    ASInt32         result;                             /* Numeric result, unique to worker type. But "zero" is always "No Problem" */
    time_t          startTime, endTime;                 /* Used in Unix only, wall time started/stopped */
    clock_t         startCPU, endCPU;                   /* Used in Unix only to track CPU time used. */
    double          wallTimeUsed, cpuTimeUsed;          /* Walltime start to finish, and CPU time (user and kernal) consumed */
    bool            silent;                             /* When true, write nothing to stdout! */
    bool            noAPDFL;                            /* When true, do not init/term the library in this thread! */
    bool            threadCompleted;                    /* Mark the thread complete for unix to locate it */
    FILE           *logFile;                            /* Write status message to this file */
    bool            logFileSet;                         /* If log file is set, then default "silent" to "false". */
} ThreadInfo;


/* This is a base class for the worker threads
*/
class workerclass
{
public:
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


    /* When true, set "ASSetTempFileSys (ASGetRamFileSys ());" at library initialization.
    ** Default is false
    */
    bool            useTempMemFileSys;

    /* Dictionary of options for this object */
    attributes *threadAttributes;

    workerclass () { InFileCount = 0;
                     InFile2Count = 0;
                     InFile3Count = 0;
                     OutPathCount = 0;
                     silent = true;
                     noAPDFL = false;
                     useTempMemFileSys = false;
                     InFilePath = InFileName = InFileSuffix = OutFilePath = NULL;
    }
    ~workerclass () { for (int index = 0; index < InFileCount; index++) 
                          { free (InFilePath[index]); free (InFileName[index]); free (InFileSuffix[index]); }
                      for (int index = 0; index < InFile2Count; index++)
                          { free (InFile2Path[index]); free (InFile2Name[index]); free (InFile2Suffix[index]); }
                      for (int index = 0; index < InFile3Count; index++)
                          {free (InFile3Path[index]); free (InFile3Name[index]); free (InFile3Suffix[index]); }
                      if (InFilePath) free (InFilePath); 
                      if (InFileName) free (InFileName); 
                      if (InFileSuffix) free (InFileSuffix); 
                      if (InFile2Path) free (InFilePath);
                      if (InFile2Name) free (InFileName);
                      if (InFile2Suffix) free (InFileSuffix);
                      if (InFile3Path) free (InFilePath);
                      if (InFile3Name) free (InFileName);
                      if (InFile3Suffix) free (InFileSuffix);
                      for (int index = 0; index < OutPathCount; index++)
                          { free (OutFilePath[index]);}
                      if (OutFilePath) free (OutFilePath);
                      delete threadAttributes;
    }

    void WorkerThread (ThreadInfo *)
    {
        return;
    }

    char *GetInFileName (int threadSequence)
    {
        char workname[2048], *result;
        int index = threadSequence % InFileCount;
        sprintf (workname, "%s%c%s.%s", InFilePath[index], PathSep, InFileName[index], InFileSuffix[index]);
        result = (char *)malloc (strlen (workname) + 1);
        strcpy (result, workname);
        return (result);
    }

    char *GetInFile2Name (int threadSequence)
    {
        if (InFile2Count == 0)
            return (NULL);

        char workname[2048], *result;
        int index = threadSequence % InFile2Count;
        sprintf (workname, "%s%c%s.%s", InFile2Path[index], PathSep, InFile2Name[index], InFile2Suffix[index]);
        result = (char *)malloc (strlen (workname) + 1);
        strcpy (result, workname);
        return (result);
    }


    char *GetInFile3Name (int threadSequence)
    {
        if (InFile3Count == 0)
            return (NULL);

        char workname[2048], *result;
        int index = threadSequence % InFile3Count;
        sprintf (workname, "%s%c%s.%s", InFile3Path[index], PathSep, InFile3Name[index], InFile3Suffix[index]);
        result = (char *)malloc (strlen (workname) + 1);
        strcpy (result, workname);
        return (result);
    }


    char * GetOutFileName (int threadSequence, int inner = -1)
    {
        int indexOut = threadSequence % OutPathCount;
        int indexIn = threadSequence % InFileCount;
        char workname[2048], *result;;
        if (inner == -1)
            sprintf (workname, "%s%c%s_%01d.%s", OutFilePath[indexOut], PathSep, InFileName[indexIn], threadSequence + 1, InFileSuffix[indexIn]);
        else
            sprintf (workname, "%s%c%s_%01d_%01d.%s", OutFilePath[indexOut], PathSep, InFileName[indexIn], threadSequence + 1, inner + 1, InFileSuffix[indexIn]);
        result = (char *)malloc (strlen (workname) + 1);
        strcpy (result, workname);
        return (result);
    }

    void startThreadWorker (ThreadInfo *info)
    {
#ifndef WIN_PLATFORM
        info->startTime = time (&info->startTime);
        info->startCPU = clock ();
#endif
        if (noAPDFL)
            info->instance = NULL;
        else
        {
            info->instance = new APDFLib ();
            if (useTempMemFileSys)
                ASSetTempFileSys (ASGetRamFileSys ());
        }
        info->silent = silent;
    }

    void endThreadWorker (ThreadInfo *info)
    {
        if (info->instance)
            delete info->instance;

#ifndef WIN_PLATFORM
        info->endTime= time (&info->endTime);
        info->endCPU = clock ();
        info->wallTimeUsed = ((info->endTime - info->startTime) * 1.0) / CLOCKS_PER_SEC;
        info->cpuTimeUsed = ((info->endCPU - info->startCPU) * 1.0) / CLOCKS_PER_SEC;
#endif

        info->threadCompleted = true;
    }


    void splitpath (char *path, char **toPath, char **filename, char **suffix)
    {
        static char pathCopy[2048];
        strcpy (pathCopy, path);
        char *finder = pathCopy + strlen (pathCopy) - 1;
        *suffix = *filename = *toPath = NULL;
        while (finder != pathCopy)
        {
            while ((finder > pathCopy) && (finder[0] != '.') & (finder[0] != PathSep))
                finder--;
            if (finder[0] == '.')
            {
                *suffix = finder + 1;
                finder[0] = 0;
                finder--;
                continue;
            }
            if (finder[0] == PathSep)
            {
                *filename = finder + 1;
                finder[0] = 0;
                finder--;
                *toPath = pathCopy;
            }
            break;
        }
    }

    /* If we get here, and the file name start with a 
    ** percent sign, then expand the name by preceedingit with the
    ** standard path to sample input
    */
    char *expandFileName (char *inname)
    {
        char *result;
        if (inname[0] == '%')
        {
            result = (char *)malloc (strlen (inname) + strlen (inputPath) + 1);
            sprintf (result, "%s%c%s", inputPath, PathSep, &inname[1]);
        }
        else
        {
            result = (char *)malloc (strlen (inname) + 1);
            strcpy (result, inname);
        }
        return (result);
    }

    void parseOneFileName (int index, char *inname)
    { 
        char *expandedName = expandFileName (inname);

        char *path, *name, *suffix;
        workerclass::splitpath (expandedName, &path, &name, &suffix);

        free (expandedName);

        InFilePath[index] = (char *)malloc (strlen (path) + 1);
        strcpy (InFilePath[index], path);
        InFileName[index] = (char *)malloc (strlen (name) + 1);
        strcpy (InFileName[index], name);
        InFileSuffix[index] = (char *)malloc (strlen (suffix) + 1);
        strcpy (InFileSuffix[index], suffix);
    }

    void parseTwoFileName (int index, char *inname)
    {
        char *expandedName = expandFileName (inname);

        char *path, *name, *suffix;
        workerclass::splitpath (expandedName, &path, &name, &suffix);

        free (expandedName);

        InFile2Path[index] = (char *)malloc (strlen (path) + 1);
        strcpy (InFilePath[index], path);
        InFile2Name[index] = (char *)malloc (strlen (name) + 1);
        strcpy (InFileName[index], name);
        InFile2Suffix[index] = (char *)malloc (strlen (suffix) + 1);
        strcpy (InFile2Suffix[index], suffix);
    }

    void parseThreeFileName (int index, char *inname)
    {
        char *expandedName = expandFileName (inname);

        char *path, *name, *suffix;
        workerclass::splitpath (expandedName, &path, &name, &suffix);

        free (expandedName);

        InFile3Path[index] = (char *)malloc (strlen (path) + 1);
        strcpy (InFilePath[index], path);
        InFile3Name[index] = (char *)malloc (strlen (name) + 1);
        strcpy (InFile3Name[index], name);
        InFile3Suffix[index] = (char *)malloc (strlen (suffix) + 1);
        strcpy (InFile3Suffix[index], suffix);
    }

    void    ParseOptions (valuelist *values, char *defaultInFileName, char *defaultOutFilePath)
    {

        if (values)
        {
            /* Parse the attributes as is there were a command line */
            int listSize = values->size ();
            char **valueArray = (char **)malloc ((listSize + 1)* sizeof (char *));
            valueArray[0] = NULL;
            for (int count = 0; count < values->size (); count++)
                valueArray[count + 1] = values->value (count);
            threadAttributes = new attributes (listSize + 1, valueArray);
        }
        else
        {
            char *nullPtr = NULL;
            threadAttributes = new attributes (1, &nullPtr);
        }

        if (threadAttributes->IsKeyPresent ("InFileName"))
        {
            valuelist *nameValues = threadAttributes->GetKeyValue ("InFileName");
            InFileCount = nameValues->size ();
            InFilePath = (char **)malloc (sizeof (char*) * InFileCount);
            InFileName = (char **)malloc (sizeof (char*) * InFileCount);
            InFileSuffix = (char **)malloc (sizeof (char*) * InFileCount);

            for (int index = 0; index < InFileCount; index++)
                parseOneFileName (index, nameValues->value (index));
        }
        else
        {
            InFileCount = 1;
            InFilePath = (char **)malloc (sizeof (char*));
            InFileName = (char **)malloc (sizeof (char*));
            InFileSuffix = (char **)malloc (sizeof (char*));
            parseOneFileName (0, defaultInFileName);
        }

        if (threadAttributes->IsKeyPresent ("InFile2Name"))
        {
            valuelist *nameValues = threadAttributes->GetKeyValue ("InFile2Name");
            InFile2Count = nameValues->size ();
            InFile2Path = (char **)malloc (sizeof (char*) * InFile2Count);
            InFile2Name = (char **)malloc (sizeof (char*) * InFile2Count);
            InFile2Suffix = (char **)malloc (sizeof (char*) * InFile2Count);

            for (int index = 0; index < InFileCount; index++)
                parseTwoFileName (index, nameValues->value (index));
        }

        if (threadAttributes->IsKeyPresent ("InFile3Name"))
        {
            valuelist *nameValues = threadAttributes->GetKeyValue ("InFile3Name");
            InFile3Count = nameValues->size ();
            InFile3Path = (char **)malloc (sizeof (char*) * InFile3Count);
            InFile3Name = (char **)malloc (sizeof (char*) * InFile3Count);
            InFile3Suffix = (char **)malloc (sizeof (char*) * InFile3Count);

            for (int index = 0; index < InFileCount; index++)
                parseThreeFileName (index, nameValues->value (index));
        }

        if (threadAttributes->IsKeyPresent ("OutFilePath"))
        {
            valuelist *nameValues = threadAttributes->GetKeyValue ("OutFilePath");
            OutPathCount = nameValues->size ();
            OutFilePath = (char **)malloc (sizeof (char *) * OutPathCount);
            for (int index = 0; index < OutPathCount; index++)
            {
                char *value = nameValues->value (index);
                OutFilePath[index] = (char *)malloc (strlen (value) + 1);
                strcpy (OutFilePath[index], value);
            }
        }
        else
        {
            OutFilePath = (char **)malloc (sizeof (char *));
            OutPathCount = 1;
            OutFilePath[0] = (char *)malloc (strlen (defaultOutFilePath) + 1);
            strcpy (OutFilePath[0], defaultOutFilePath);
        }

        if (threadAttributes->IsKeyPresent ("Silent"))
            silent = threadAttributes->GetKeyValueBool ("Silent");

        if (threadAttributes->IsKeyPresent ("NoAPDFL"))
            noAPDFL = threadAttributes->GetKeyValueBool ("NoAPDFL");

        if (threadAttributes->IsKeyPresent ("TempMemFileSys"))
            useTempMemFileSys = threadAttributes->GetKeyValueBool ("TempMemFileSys");


        char workpath[2048];
        for (int index = 0; index < InFileCount; index++)
        {
            sprintf (workpath, "%s%c%s.%s", InFilePath[index], PathSep, InFileName[index], InFileSuffix[index]);
            if (access (workpath, 04))
            {
                printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
                exit (-1);
            }
        }

        for (int index = 0; index < InFile2Count; index++)
        {
            sprintf (workpath, "%s%c%s.%s", InFile2Path[index], PathSep, InFile2Name[index], InFile2Suffix[index]);
            if (access (workpath, 04))
            {
                printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
                exit (-1);
            }
        }

        for (int index = 0; index < InFile3Count; index++)
        {
            sprintf (workpath, "%s%c%s.%s", InFile3Path[index], PathSep, InFile3Name[index], InFile3Suffix[index]);
            if (access (workpath, 04))
            {
                printf ("The input file does not exist? \n%   \"%s\"\n", workpath);
                exit (-1);
            }
        }

        for (int index = 0; index < OutPathCount; index++)
        {
            if (access (OutFilePath[index], 6))
            {
                mkdir (OutFilePath[index], 666);
                if (access (OutFilePath[index], 6))
                {
                    printf ("The output path cannot be found or created!\n    \"%s\"\n", OutFilePath);
                    exit (-1);
                }
            }
        }
    }


};

