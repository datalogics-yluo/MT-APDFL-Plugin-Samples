/*
** PDF/x worker
** This thread will convert one document to PDF/x.
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
#include "MTHeader.h"
#include "Worker.h"
#include "PDFProcessorCalls.h"
#include "CosCalls.h"
#include "ASCalls.h"
#include "Utilities.h"
class PDFxWorker : public workerclass
{
#define NumberOfPDFXConvertOptions 2
public:
    PDFxWorker ()
    {
        workerType = PDFX;
        ConvertOptions[0] = kPDFProcessorConvertToPDFX1a2001;
        ConvertNames[0] = "PDFX1A";
        ConvertOptions[1] = kPDFProcessorConvertToPDFX32003;
        ConvertNames[1] = "PDFX3";
    };
    ~PDFxWorker () { };

    /* Parse the PDF/x conversion thread options into attributes
    **  RemoveAllAnnotations is true/false, and is passed into the PDF/a convertor
    **  ConvertOption is one of PDFX1A, PDFX3.
    **  UseProgressMonitor will turn on or off the progress monitor Default is off
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

private:

    bool removeAllAnnotations[100];
    int  removeAllAnnotationsCount;

    ASUns32 convertorOptions[100];
    int  convertorOptionsCount;

    bool useProgressMonitor;

    PDFProcessorPDFXConversionOption ConvertOptions[NumberOfPDFXConvertOptions];
    char *ConvertNames[NumberOfPDFXConvertOptions];
};