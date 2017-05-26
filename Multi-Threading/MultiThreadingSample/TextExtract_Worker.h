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
**                   SaveWordList=false                                 If true, the words will be written to a file.If false, not. 
*/
#include "MTHeader.h"
#include "Worker.h"

class TextextWorker : public workerclass
{
public:
    TextextWorker () 
    { 
        workerType = TextExtract; 
    };
    ~TextextWorker () { };

/* Parse the TextExtract  thread options into attributes
**  Number Of Pages is the number of pages to render in one thead. It may be a list.
*/
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

private:
    ASUns32 pages[100];
    ASUns32 pagesCount;
    bool    saveWordList;

};
