/*
** PDF/a worker
** This thread will convert one document to PDF/a.
** NoAPDFL MUST be false, or the thread will not function.
** InFileName as a list will use the Nth entry of the list, modulus the number of entries in the list, for each worker thread.

**  PDFaOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\AddRedactions.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   noAPDFL=false                                      When true, we will NOT init/term the library for each thread
**                   ConvertOption=[1]                                  Convertor Option selector, One of [RGB1a | RGB1b | CMYK1a | CMYK1b] (100 values max!)
**                   RasterizeFontErrors=[false]                        SettingforPDF/a conversion option "rasterizeFontErrors" (100 values max!)
**                   RemoveAllAnnotations=[false]                       SettingforPDF/a conversion option "removeAllAnnotations" (100 values max!)
**                   UseProgressMonitor=false                           Uses or does not use the progress monitor. Singular Value! Default is off.
*/
#include "MTHeader.h"
#include "Worker.h"
#include "PDFProcessorExpT.h"

class PDFaWorker : public workerclass
{
#define NumberOfPDFAConvertOptions 4

public:

    PDFaWorker ()
    {
        workerType = PDFA;
        ConvertOptions[0] = kPDFProcessorConvertToPDFA1aRGB;
        ConvertorNames[0] = "RGB1A";
        ConvertOptions[1] = kPDFProcessorConvertToPDFA1aCMYK;
        ConvertorNames[1] = "CMYK1A";
        ConvertOptions[2] = kPDFProcessorConvertToPDFA1bRGB;
        ConvertorNames[2] = "RGB1B";
        ConvertOptions[3] = kPDFProcessorConvertToPDFA1bCMYK;
        ConvertorNames[3] = "CMYK1B";
        useProgressMonitor = false;
    };

    ~PDFaWorker ()
    { };

    /* Parse the PDF/a conversion thread options into attributes
    **  RasterizeFontErrors is true/false, and is passed into the PDF/a convertor
    **  RemoveAllAnnotations is true/false, and is passed into the PDF/a convertor
    **  ConvertOption is one of RGB1a, RGB1b, CMYK1a, CMYK1b.
    **  UseProgressMonitor will turn on or off the progress monitor Default is off
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    void WorkerThread (ThreadInfo *info);

private:

    bool rasterizeFontErrors[100];
    int  rasterizeFontErrorsCount;
    bool removeAllAnnotations[100];
    int  removeAllAnnotationsCount;
    ASUns32 convertorOptions[100];
    int  convertorOptionsCount;
    bool useProgressMonitor;

    PDFProcessorPDFAConversionOption ConvertOptions[NumberOfPDFAConvertOptions];
    char *ConvertorNames[NumberOfPDFAConvertOptions];
};