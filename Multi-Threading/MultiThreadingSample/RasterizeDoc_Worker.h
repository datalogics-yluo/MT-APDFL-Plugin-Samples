/*
** RasterizeDoc Worker
** This worker type will open a document, and create a new document.
** For each page in the original document, it will rasterize the page, and add the raster as an image to the 
** new document.
**
** If "NoAPDFL=true" must not be specified for this worker type!
**
**  RasterizeDocOptions=[
**                   silent=true                                        When true, do not display status lines
**                   InFileName=[test data path\constitution.pdf]      A list of file to copy, One file will be copied by each thread, sequcence % #files.
**                   OutFilePath=[Output]                               Directory where output is written
**                   SaveOutput=true                                    When true, we will save the newly created image document
**                   Resolution=300                                     Resolution to render image too.
**                   ColorModel={RGB,CMYK,GRAY,DeviceN]                 Which color model to use. RGB is the default.
*/
#include "Worker.h"

class RasterizeDocWorker : public workerclass
{
    /* This thread will render a one or more pages from a document, 1 or more times, saving (optionally) the
    ** result in a PDF file. Eachthread will render the page that follows the page(s) rendered in the previous
    ** thread, wrapping back to the document start if there are more renderings than pages in the document.
    */
public:
    RasterizeDocWorker ()
    {
        workerType = RasterizeDoc;
    };
    ~RasterizeDocWorker () { };

    /* Parse the RasterizeDocWorker conversion thread options into attributes
    **  SaveOuput is true or false. If true, a PDF document containing all renders will be created. If false, it will not be.
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
    ASBool      saveOutput;
    double      Resolution;
    char        colorModel[20];
    ASInt8      colorComponents;
};