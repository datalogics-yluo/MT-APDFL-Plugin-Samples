/*
** Rasterizer Worker
** This worker type will open a document, rasterize one or more pages, one or more times, Optionally placing the
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
#include "Worker.h"

class RasterizerWorker : public workerclass
{
    /* This thread will render a one or more pages from a document, 1 or more times, saving (optionally) the
    ** result in a PDF file. Eachthread will render the page that follows the page(s) rendered in the previous
    ** thread, wrapping back to the document start if there are more renderings than pages in the document.
    */
public:
    RasterizerWorker () 
    { 
        workerType = Rasterizer; 
    };
    ~RasterizerWorker () { };

    /* Parse the PDF/x conversion thread options into attributes
    **  Number Of Pages is the number of pages to render in one thead. It may be a list.
    **  Repetitions is the number of times to render each page. It may be a list.
    **  SaveImages is true or false. If true, a PDf document containing all renders will be created. If false, it will not be.
    **      This value may be a list, but only the first entry will be used.
    **  Resolution sets the render resolution, in Dots Per Inch. 
    **  Color model selects the color to render too. It may be one of "DeviceRGB", "DeviceCMYK", or "DeviceGray".
    */
    void ParseOptions (attributes *FrameAttributes, WorkerType *worker);

    /* This is a utility routine to render on page to a bitmap */
    char *RenderPageToBitmap (PDPage page, ASSize_t *mapSize, ASSize_t *width, ASSize_t *depth);

    /* This is a utility routine to add one bitmap to a document */
    void AddImageToDoc (PDDoc doc, size_t mapSize, char *map, size_t width, size_t depth, ThreadInfo *info);

    void WorkerThread (ThreadInfo *info);


private:
    ASInt32     Repetitions[100];
    ASInt32     RepetitionsCount;
    ASInt32     pages[100];
    ASInt32     pagesCount;
    ASBool      saveImages;
    double      Resolution;
    char        colorModel[20];
    ASInt8      colorComponents;
};
