// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//=====================================================================
// This header file defines classs used in the multiThreading sample.
#ifndef MTHEADER_H
#define MTHEADER_H

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

#include "ASExpT.h"


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
#define inputPath "..\\..\\Resources\\Sample_Input"
#else
#define PathSep  '/'
#define inputPath "../../Resources/Sample_Input"
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
                    size_t length = end - start;
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

    int size () { return (int)list.size (); }

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
#if !MAC_ENV
        if ((argc == 2) && (argv[0] != NULL))
#else
        // mac there are two extra dummy args, Argument 4 is the readable file name
        if ((argc == 4) && (argv[0] != NULL))
#endif
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
                    size_t keyLen = equal - argv[count];
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


#endif
