/*
** Rasterizer Worker
** This worker type will open a document, rasterize one or more pages, one or more times, Optoiopnally placiong the
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
#include "Rasterizer_Worker.h"
#include "PDPageDrawM.h"
#include "DLExtrasCalls.h"
#include "PEWCalls.h"
#include "PERCalls.h"
#include "PagePDECntCalls.h"



/* Parse the PDF/x conversion thread options into attributes
**  Number Of Pages is the number of pages to render in one thead. It may be a list.
**  Repetitions is the number of times to render each page. It may be a list.
**  SaveImages is true or false. If true, a PDf document containing all renders will be created. If false, it will not be.
**      This value may be a list, but only the first entry will be used.
**  Resolution sets the render resolution, in Dots Per Inch.
**  Color model selects the color to render too. It may be one of "DeviceRGB", "DeviceCMYK", or "DeviceGray".
*/
void RasterizerWorker::ParseOptions (attributes *FrameAttributes, WorkerType *worker)
{
    /* Fill in the worker interface table for this worker type */
    WorkerIDEntry = worker;
    worker->name = "Rasterizer";
    worker->LoadPlugins = false;
    worker->paramName = "RasterizerOptions";
    worker->type = workerType;

    /* Parse the common attributes for this worker type,
    ** Provide defaults for InFileName and OutFilePath
    */
    workerclass::ParseOptions (FrameAttributes, "%AddRedaction.pdf", "Output");

    /* How many times should I render each page.
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

    /* How many pages shall I render
    */
    if (threadAttributes->IsKeyPresent ("NumberOfPages"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("NumberOfPages");
        pagesCount = values->size ();
        for (ASInt32 index = 0; index < pagesCount; index++)
            pages[index] = values->GetValueInt (index);
    }
    else
    {
        pagesCount = 1;
        pages[0] = 1;
    }

    /* Shall I save the results of rendering?
    **  Saving the results of rendering tends to distort measuring, since most of the 
    ** cases where we render in mutli-threading, we do not save the result to a file, 
    ** but display it to a screen. so generally, we want this to be false. We would want it 
    ** to be true if we need to examine the rendered page to validate correctness.
    */
    if (threadAttributes->IsKeyPresent ("SaveImages"))
        saveImages = threadAttributes->GetKeyValueBool ("SaveImages");
    else
        saveImages = false;

    /* To what resolution shall I render the page
    ** This value is singular, and will apply to all renderers built. 
    ** It is concievable that some applications may mix resolutions, and if that 
    ** need to be done, it can be.
    */
    if (threadAttributes->IsKeyPresent ("Resolution"))
        Resolution = threadAttributes->GetKeyValueDouble ("Resolution");
    else
        Resolution = 300.0;

    /* To what color modesl shall we render. 
    ** This also is singular. At this point, only 
    ** the three device colors are supported. It is possible 
    ** to extend the model to support profiled colors, or other 
    ** color models.
    */
    if (threadAttributes->IsKeyPresent ("ColorModel"))
    {
        valuelist *values = threadAttributes->GetKeyValue ("ColorModel");
        char *name = values->value (0);
        for (int i = 0; name[i] != 0; i++)
            name[i] = toupper (name[i]);
        if (!strcmp (name, "DEVICEN"))
        {
            strcpy (colorModel, name);
            colorComponents = 4;
        }
        else
        {
            if (!strcmp (name, "DEVICEGRAY"))
            {
                strcpy (colorModel, name);
                colorComponents = 1;
            }
            else
            {
                if (!strcmp (name, "DEVICECMYK"))
                {
                    strcpy (colorModel, name);
                    colorComponents = 4;
                }
                else
                {
                    strcpy (colorModel, "DeviceRGB");
                    colorComponents = 3;
                }
            }
        }
    }
    else
    {
        strcpy (colorModel, "DeviceRGB");
        colorComponents = 3;
    }

};

char *RasterizerWorker::RenderPageToBitmap (PDPage page, ASSize_t *mapSize, ASSize_t *width, ASSize_t *depth)
{

    /* Get the matrix that transforms user space coordinates to rotated and cropped upright image coordinates.
    **  The origin of this space is the top-left of the rotated, cropped page. Y is decreasing
    */
    ASFixedMatrix matrix;
    PDPageGetFlippedMatrix (page, &matrix);

    //Gets the crop box for a page. The crop box is the region of the page to display and print
    ASFixedRect pageRect;
    PDPageGetCropBox (page, &pageRect);

    /* Set the scale matrix that will be concatenated to the user space matrix
    ** to accomplish the desired resolution
    */
    ASFixedMatrix scaleMatrix = { FloatToASFixed (Resolution / 72.0), 0, 0, FloatToASFixed (Resolution / 72.0), 0, 0 };

    //Apply the scale to the default matrix
    ASFixedMatrixConcat (&matrix, &scaleMatrix, &matrix);

    /* Apply scale to the destination rectangle*/
    ASFixedRect destRect;
    ASFixedMatrixTransformRect (&destRect, &scaleMatrix, &pageRect);

    /* If the page is offset, remove the offset here */
    if (destRect.left != 0)
    {
        destRect.right -= destRect.left;
        destRect.left = 0;
    }
    if (destRect.bottom != 0)
    {
        destRect.top -= destRect.bottom;
        destRect.bottom = 0;
    }

    /* Generate width and depth */

    /* Construct the draw params record */
    PDPageDrawMParamsRec drawParams;
    memset ((char *)&drawParams, 0, sizeof (PDPageDrawMParamsRec));
    drawParams.size = sizeof (PDPageDrawMParamsRec);
    drawParams.destRect = &destRect;
    drawParams.matrix = &matrix;
    drawParams.csAtom = ASAtomFromString (colorModel);
    drawParams.bpc = 8;
    drawParams.flags = kPDPageDoLazyErase;

    /* Call draw to memory to get buffer size */
    size_t bufferSize = PDPageDrawContentsToMemoryWithParams (page, &drawParams);

    /* If we are doing deviceN, pickup the number of inks here */
    if (drawParams.deviceNColorCount)
        colorComponents = drawParams.deviceNColorCount[0];

    /* If bufferSize is zero, then we set up inks inthe previous call (DeviceN)
    ** So call again to get buffer size
    */
    if (bufferSize == 0)
        bufferSize = PDPageDrawContentsToMemoryWithParams (page, &drawParams);

    /* if buffer size is still zero, something is wrong. Error out!
    */
    if (bufferSize == 0)
        return (NULL);

    /* Allocate a buffer to hold the bitmap */
    char *buffer = (char *)malloc (bufferSize);

    /* Point to it int he draw params.*/
    drawParams.buffer = buffer;
    drawParams.bufferSize = bufferSize;

    /* Draw the page !*/
    PDPageDrawContentsToMemoryWithParams (page, &drawParams);

    /* Tell the caller the size of the map, width, and depth */
    *mapSize = bufferSize;
    *width = abs (ASFixedRoundToInt16 (destRect.right) - ASFixedRoundToInt16 (destRect.left));
    *depth = abs (ASFixedRoundToInt16 (destRect.top) - ASFixedRoundToInt16 (destRect.bottom));

    /* Return the map to the caller */
    return (buffer);
}

void RasterizerWorker::AddImageToDoc (PDDoc doc, size_t mapSize, char *map, size_t width, size_t depth, ThreadInfo *info)
{
    /* Set upimage Attributes.
    ** Always an XObject image
    */
    PDEImageAttrs attrs;
    memset ((char *)&attrs, 0, sizeof (PDEImageAttrs));
    attrs.flags = kPDEImageExternal;
    attrs.width = width;
    attrs.height = depth;
    attrs.bitsPerComponent = 8;

    /* Always compress image flate */
    PDEFilterArray filterArray;
    memset (&filterArray, 0, sizeof (PDEFilterArray));
    filterArray.numFilters = 1;
    filterArray.spec[0].name = ASAtomFromString ("FlateDecode");

    /* Image is created passed to a 32 bit boundary per row.
    ** For image usage, it must be padded to 8 bits per row
    */
    ASUns32 rowWidthPacked = width * colorComponents;
    ASUns32 rowWidthPadded = (((width * colorComponents * 8) + 31) / 32) * 4;

    /* Repack rows, as needed */
    if (rowWidthPacked != rowWidthPadded)
    {
        /* Remove the row padding bytes */
        for (ASUns16 row = 1; row < depth; row++)
            memmove (&map[row * rowWidthPacked], &map[row * rowWidthPadded], rowWidthPacked);
    }

    /* New size */
    size_t length = width * colorComponents * depth;

    /* Image is erect, and sized 1 point per pixel */
    ASFixedMatrix matrix = { width * fixedOne, 0, 0, depth * fixedOne, 0, 0 };

    /* Color space as per creation
    ** (NOTE: This will not work for deviceN color spaces, will have to
    **  add logic to create a color space from ink table is we want to support
    ** deviceN colors. Too much to do for now.
    */
    PDEColorSpace cs = PDEColorSpaceCreateFromName (ASAtomFromString (colorModel));

    /* Create a PDE Image (One that can be added to a PDF page */
    PDEImage image = PDEImageCreate (&attrs, sizeof (attrs), &matrix, 0, cs, NULL, &filterArray, NULL, (ASUns8*)map, length);

    /* Create a page to hold the image
    ** Just the size of the image.
    */
    ASFixedRect pageSize = { 0, depth * fixedOne, width * fixedOne, 0 };
    PDPage page = PDDocCreatePage (doc, PDDocGetNumPages (doc) - 1, pageSize);

    /* Get the PDE Content */
    PDEContent content = PDPageAcquirePDEContent (page, 0);

    /* Set the image into the page */
    PDEContentAddElem (content, kPDEAfterLast, (PDEElement)image);

    /* Bind he content back into the page */
    PDPageSetPDEContent (page, 0);

    /* Release resources */
    PDERelease ((PDEObject)image);
    PDPageReleasePDEContent (page, 0);
    PDERelease ((PDEObject)cs);

}

void RasterizerWorker::WorkerThread (ThreadInfo *info)
{
    int sequence = info->sequence;
    if (!silent)
        fprintf (info->logFile, "Rasterizer Worker Thread Started! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);

    /* Generate input name */
    char *fullFileName = GetInFileName (sequence);


    DURING
        /* Open the input document */
        APDFLDoc inDoc (fullFileName, true);
    PDDoc pdDoc = inDoc.getPDDoc ();


    /* Get the number of pages */
    size_t pagesInDocument = PDDocGetNumPages (pdDoc);

    /* How many pages have we already done*/
    ASUns64 numberOfPagesDone = 0;
    for (int index = 0; index < sequence; index++)
        numberOfPagesDone += pages[index % pagesCount];

    /* reduce modulos the number of pages in the document */
    ASUns32 firstPageToDo = numberOfPagesDone % pagesInDocument;

    ASUns32 numberOfPagesToDo = pages[sequence % pagesCount];

    PDDoc outDoc;
    if (saveImages)
        outDoc = PDDocCreate ();
    else
        outDoc = NULL;

    /* Do the pages fromfirst to last, N times */
    for (ASInt32 loop = 0; loop < Repetitions[sequence % RepetitionsCount]; loop++)
    {
        /* Do the requires set of pages */
        for (ASInt32 indexPage = 0; indexPage < pages[sequence % pagesCount]; indexPage++)
        {
            /* Obtain the current page */
            ASUns32 pageToDo = (firstPageToDo + indexPage) % pagesInDocument;
            PDPage page = PDDocAcquirePage (inDoc.getPDDoc (), pageToDo);

            /* Render the current page */
            ASSize_t mapsize, width, depth;
            char *mapBuffer = RenderPageToBitmap (page, &mapsize, &width, &depth);

            /* Release the current page */
            PDPageRelease (page);

            /* If we are saving the images, make the map and image, and
            ** add it to the images document
            */
            if (saveImages)
            {
                AddImageToDoc (outDoc, mapsize, mapBuffer, width, depth, info);
            }

            /* Free the bitmap */
            free (mapBuffer);

        }
    }

    if (saveImages)
    {
        /* The automatic logic will use he same suffix for the output as the input, so change the suffix here */
        char *fullOutputFileName = GetOutFileName (sequence, -1);
#if !MAC_ENV	
        ASPathName destFilePath = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), fullOutputFileName, NULL);
#else
        ASPathName destFilePath = APDFLDoc::makePath (fullOutputFileName);
#endif

        PDDocSave (outDoc, PDSaveFull | PDSaveCollectGarbage, destFilePath, NULL, NULL, NULL);
        ASFileSysReleasePath (NULL, destFilePath);
        free (fullOutputFileName);
        PDDocClose (outDoc);
    }

    HANDLER
        info->result = 1;
    END_HANDLER

        if (!silent)
            fprintf (info->logFile, "Rasterizer Worker Thread completed! (Sequence: %01d, Thread: %01d\n", sequence + 1, info->threadNumber + 1);
}

