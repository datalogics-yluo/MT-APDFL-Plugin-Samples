/*
//
//  ADOBE SYSTEMS INCORPORATED
//  Copyright (C) 2000-2003 Adobe Systems Incorporated
//  All rights reserved.
//
//  NOTICE: Adobe permits you to use, modify, and distribute this file
//  in accordance with the terms of the Adobe license agreement
//  accompanying it. If you have received this file from a source other
//  than Adobe, then your use, modification, or distribution of it
//  requires the prior written permission of Adobe.
//
*/

#include "FlattenPDFWorker.h"
#include "WatchFolder.h"
#include "stdio.h"
//PDFL Headers
#include "PDFInit.h"
#include "ASCalls.h"
#include "PDCalls.h"
#include "SmartPDPage.h"
#include "PDFLExpT.h"
#include "ASExtraCalls.h"


////////////////////
// Flattener Plugin headers
#include "PDFlattenerCalls.h"

#ifdef UNIX_ENV
#include "string.h"
#endif
void FlattenPDFMain(const char * input, char * output, FlattenProgressMonitor = 0, void * clientData = 0);
ASBool FlattenProgressMonitorCB(ASInt32 pageNum, ASInt32 totalPages, float current, ASInt32 reserved, void *clientData);
ThreadFuncReturnType FlattenPDF(ThreadArgs *pArgs)
{
	INIT_AUTO_POOL(autoReleasePool);	/* Required only on MAC platform */
	
	WatchFolder *theWF = pArgs->watchFolder;

	while (1){
		// we intitialise inside the loop to remove the risk of internal library leaks
		// this has the effect of deleting all the thread specific PDFL data.
		MyPDFLInit();
		
		// first thing we do is get the path from the WatchFolder

		char* fileToExtract;
		if(theWF->numToReturn > 0)
			fileToExtract = (char*) theWF->getFile();
		else 
			fileToExtract = NULL;

		// we terminate the main loop if the watchFolder abstraction
		// gives us a NULL token. This thread can now die.
		if (fileToExtract == NULL) {
			MyPDFLTerm();
			/////break;
			return 0;
		}

		// we have a valid file to extract the text from.
		// the destination file is filenmae.txt, in the parent directory.
		// this code is intentionally simple, a real implementation would
		// have a robust implementation.
		string tmpstr(fileToExtract);
		ASSize_t sentinal = tmpstr.rfind('/');
		string filename = tmpstr.substr(sentinal,string::npos);
		filename.replace(filename.end() - 4, filename.end(), "-out.pdf");
		tmpstr = tmpstr.substr(0,sentinal);
		sentinal = tmpstr.rfind('/');
		tmpstr = tmpstr.substr(0,sentinal);
		tmpstr += string("/outdir") + filename;
		char *pathnm = static_cast<char *>(ASmalloc(tmpstr.length()+1));
		sprintf_safe(pathnm,tmpstr.length()+1,"%s",tmpstr.c_str());
		

		char * newFilename = NULL;		
		// need a local copy of our filename to get around constness.
		newFilename = static_cast<char *>(ASmalloc(strlen(fileToExtract)+1));
		strcpy_safe(newFilename, strlen(fileToExtract)+1, fileToExtract);

		char inputbuffer[400], outputbuffer[400];
        sprintf_safe(inputbuffer, sizeof(inputbuffer), newFilename);
        printf("\n%s\n", inputbuffer);

        sprintf_safe(outputbuffer, sizeof(outputbuffer), pathnm);
        printf("%s\n", outputbuffer);

        DURING
            gPDFlattenerHFT = InitPDFlattenerHFT;

        //initialize flattener plugin
        if (gPDFlattenerHFT  &&   PDFlattenerInitialize())
        {
            //Flatten PDF using API PDFlattenerConvertEx() with Callback
            printf("\nFlattening using PDFlattenerConvertEx (with Callback)\n");
            ASBool progressData = false;
            FlattenPDFMain(inputbuffer, outputbuffer,
                FlattenProgressMonitorCB, /* Progress Monitor Callback with Cancel Option */
                (void*)&progressData /* Client Supplied Data for Callback*/);

            //Terminate flattener plugin
            PDFlattenerTerminate();
        }
        else
            /* DLADD: kshahn 13Aug2009 - Plugin is PDFlattener. */
            printf("Initialization of PDFlattener Plugin failed...\n");
        HANDLER
            printf("MainProc has thrown an exception...\n");
        DisplayError(ERRORCODE);
        END_HANDLER

		MyPDFLTerm();
	}
	
	RELEASE_AUTO_POOL(autoReleasePool); /* Required only on MAC platform */
	
	return 0;
}

//Progress Monitor CallBack for Flattener
ASBool FlattenProgressMonitorCB(ASInt32 pageNum, ASInt32 totalPages, float current, ASInt32 reserved, void *clientData)
{
    if (clientData)
    {
        ASBool * IsMonitorCalled = (ASBool*)clientData;
        if (*IsMonitorCalled == false)
        {
            printf("Flattening Progress Monitor CallBack\n");
            //Set to true to Display this Message Only Once
            *IsMonitorCalled = true;
        }
    }

    printf("Flattening Page %d of %d. Overall Progress = %f %%. \n",
        pageNum + 1, /* Adding 1, since Page numbers are 0-indexed*/
        totalPages,
        current /* Current Overall Progress */);

    //Return 1 to Cancel Flattening
    return 0;
}
void FlattenPDFMain(const char * input, char * output, FlattenProgressMonitor, void * clientData)
{
    DURING
    PDDoc pddoc = NULL;

    string tmpstr(input);
    ASSize_t sentinal = tmpstr.rfind('/');
    string filename = tmpstr.substr(sentinal,string::npos);
    filename = string("indir") + filename;
    char *pathnm = static_cast<char *>(ASmalloc(filename.length() + 1));
    sprintf_safe(pathnm, filename.length() + 1, "%s", filename.c_str());

    pddoc = MyPDDocOpen(pathnm);
    CSmartPDPage onePage;

    if (0)
    {
        onePage.AcquirePage(pddoc, 0);

        if (PDPageHasTransparency(onePage, true))
            printf("YL -- this page has transparency \n");
        onePage.Reset();
    }

    ASInt32 firstPage, lastPage;
    firstPage = 0;
    lastPage = PDDocGetNumPages(pddoc) - 1;

    //set up parameters for PDFlattenerConvert
    PDFlattenerUserParamsRec flattenUserParams;
    PDFlattenRec flattenParams;
    memset(&flattenParams, 0, sizeof(PDFlattenRec));
    flattenParams.size = sizeof(PDFlattenRec);
    memset(&flattenUserParams, 0, sizeof(PDFlattenerUserParamsRec));
    flattenUserParams.size = sizeof(PDFlattenerUserParamsRec);
    flattenParams.clipComplexRegions = true;
    flattenParams.strokeToFill = true;
    flattenParams.useTextOutlines = false;
    flattenParams.preserveOverprint = true;
    flattenUserParams.transQuality = 100.0;
    flattenParams.internalDPI = 1200;
    flattenParams.externalDPI = 300;
    flattenUserParams.colorCompression = kPDFlattenerJpegCompression;
    flattenUserParams.colorImageQuality = kPDFlattenerMaximum;
    flattenUserParams.flattenParams = &flattenParams;
    flattenUserParams.monoCompression = kPDFlattenerMonoZipCompression;
    flattenUserParams.profileDesc = NULL;
    flattenUserParams.flattenProgress = FlattenProgressMonitorCB;
    flattenUserParams.progressClientData = FlattenProgressMonitorCB ? (void *)clientData : NULL;

    // flatten the transparent pdf
    ASInt32 res;
    ASUns32 numFlattenedPages;

    res = PDFlattenerConvertEx2(pddoc, firstPage, lastPage, &numFlattenedPages, &flattenUserParams);
    if (res)
    {
        printf("File %s has been successfully flattened.\n", input);
        ASPathName outPath = ASFileSysCreatePathFromDIPath(NULL, output, NULL);

        PDDocSave(pddoc, PDSaveFull | PDSaveCollectGarbage, outPath, ASGetDefaultFileSys(), NULL, NULL);
        printf("Flattened file has been successfully saved to %s.\n", output);
        if (outPath)
            ASFileSysReleasePath(NULL, outPath);

    }
    else
        printf("Flattening of file %s has failed...\n", input);

    //clean up
    PDDocClose(pddoc);
    HANDLER
        DisplayError(ERRORCODE);
    END_HANDLER
}

