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

#include "Worker.h"
#include "ASExpT.h"
class NonAPDFLWorker : public workerclass
{
/* This thread will copy a specified file N times, into new specified files.
** Each time the file is copied, it will also find the fist N prime numbers!
**
** The intent is to provide a mixture of I/O bound and CPU Bound actvities
*/
public:
    NonAPDFLWorker ()
    {
        workerType = NONAPDFL;
        Repetitions[0] = 0;
        RepetitionsCount = 0;
        Primes[0] = 0;
        PrimesCount = 0;
    };

    ~NonAPDFLWorker () { };

    /* Parse the non-APDFL conversion thread options into attributes 
    **  Repetitions is a list of numbers, telling the worker how many times to copy a file.
    **  Primes is a list of numbers, telling the worker how many print numbers to locate
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

private:

    /* Locate the first N prime numbers */
    int FindPrimes (ASUns32 *primes, ASUns32 limit);


    ASInt32     Repetitions[100];
    ASInt32     RepetitionsCount;
    ASInt32     Primes[100];
    ASInt32     PrimesCount;
};
