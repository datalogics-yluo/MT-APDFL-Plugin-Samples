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

    /* This thread will copy a specified file N times, into new specified files.
    ** Each time the file is copied, it will also find the fist N prime numbers!
    **
    ** The intent is to provide a mixture of I/O bound and CPU Bound actvities
    */

#include "NonApdfl_Worker.h"

    /* Parse the non-APDFL conversion thread options into attributes */
void NonAPDFLWorker::ParseOptions (attributes *frameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "NonAPDFL";
    worker->LoadPlugins = false;
    worker->paramName = "NonAPDFLOptions"; 
    worker->type = workerType;

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (frameAttributes, "%AddRedaction.pdf", "Output");

    /* This worker uses a list of repition counts to control how many times
    ** it will copy the input file to the output file. 
    **
    ** default is 5 times.
    */
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

    /* This worker inserts CPU Bound usage by creating a list of 
    ** prime numbers. This option controls the size of that list. 
    ** It may be a list of numbers, allowing the length of worker 
    ** threads to vary.
    ** The default is a single entry of 1000
    */
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

/* A brute force algorithm to find the first N prime numbers.
** Thisisintended to be a CPU bound load
*/
int NonAPDFLWorker::FindPrimes (ASUns32 *primes, ASUns32 limit)
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
void NonAPDFLWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;

    if (!silent)
        fprintf (info->logFile, "Non APDFL Worker Thread started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    /* Presume we will complete cleanly */
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
        /* Find file size*/
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
        fprintf (info->logFile, "Non APDFL Worker Thread completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}

