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

#include "XPS2PDFWorker.h"
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
// XPS2PDF Plugin headers
#include "XPS2PDFCalls.h"

#ifdef UNIX_ENV
#include "string.h"
#endif

#define	UsingSingleInput	        1	/* Using single inputfile TransPDF.pdf for all thread */
#define XPS_CONVERSION_OPTION_KEY		"PDFSettings"
#define XPS_CONVERSION_OPTION_KEYLANG	"PDFSettingsLang"
#define PDF_EXTENSION ".pdf"
#define PLATFORM_LANGUAGE "ENU"
#define JOB_OPTIONS_NAME "../../Resource/joboptions/Standard.joboptions"

int XPSToPDFMain(char* input, char*output);

ThreadFuncReturnType OneXPS2PDF(ThreadArgs *pArgs)
{
	INIT_AUTO_POOL(autoReleasePool);	/* Required only on MAC platform */
	
	WatchFolder *theWF = pArgs->watchFolder;
    // we intitialise outside the loop
    MyPDFLInit();
    gXPS2PDFHFT = InitXPS2PDFHFT;
    //initialize XPS2PDF plugin
    if (!XPS2PDFInitialize())
    {
        fprintf(stderr, "Error initializing XPS2PDF Plugin -- aborting\n");
        return NULL;
    }
    printf("Begin to convert one XPS 2 PDF ... \n");
	while (1){
		
		// first thing we do is get the path from the WatchFolder
		char* fileToExtract;
		if(theWF->numToReturn > 0)
			fileToExtract = (char*) theWF->getFile();
		else 
			fileToExtract = NULL;

		// we terminate the main loop if the watchFolder abstraction
		// gives us a NULL token. This thread can now die.
		if (fileToExtract == NULL) {
            //Terminate XPS2PDF plugin
            XPS2PDFTerminate();
			MyPDFLTerm();
			/////break;
			return 0;
		}

		// we have a valid file to extract the text from.
		// the destination file is filenmae.txt, in the parent directory.
		// this code is intentionally simple, a real implementation would
		// have a robust implementation.
		printf("fileToExtract -%s\n", fileToExtract);
		string tmpstr(fileToExtract);
		ASSize_t sentinal = tmpstr.rfind('/');
		string filename = tmpstr.substr(sentinal,string::npos);
		filename.replace(filename.end() - 4, filename.end(), "-out.xps");

		tmpstr = string("outdir") + filename;
		char *pathnm = static_cast<char *>(ASmalloc(tmpstr.length()+1));
		sprintf_safe(pathnm,tmpstr.length()+1,"%s",tmpstr.c_str());
		

		char * newFilename = NULL;		
		// need a local copy of our filename to get around constness.
		newFilename = static_cast<char *>(ASmalloc(strlen(fileToExtract)+1));
		strcpy_safe(newFilename, strlen(fileToExtract)+1, fileToExtract);

		char inputbuffer[400], outputbuffer[400];
#if UsingSingleInput
		sprintf_safe(inputbuffer, sizeof(inputbuffer), "xps2pdf.xps");
#else
        sprintf_safe(inputbuffer, sizeof(inputbuffer), newFilename);
#endif
        printf("\ninput - %s\n", inputbuffer);

        sprintf_safe(outputbuffer, sizeof(outputbuffer), pathnm);
        printf("output - %s\n", outputbuffer);

        DURING

        //XPS2PDF using API with Callback
        printf("\nConverting XPS2PDF (with Callback)\n");

        XPSToPDFMain(inputbuffer, outputbuffer);

        HANDLER
        printf("MainProc has thrown an exception...\n");
        DisplayError(ERRORCODE);
        END_HANDLER

	}
	
	RELEASE_AUTO_POOL(autoReleasePool); /* Required only on MAC platform */
	
	return 0;
}
int XPSToPDFMain(char* input, char*output)
{
	ASInt32 ret_val = 0;
	PDDoc outPDDoc = NULL;
	DURING
		ASCab settingsCab = ASCabNew();
	// DLADD kshahn 4Aug2009 - Next two lines: convert second param to ASHostEncoding
	ASText langText = ASTextFromEncoded(PLATFORM_LANGUAGE, ASScriptToHostEncoding(kASRomanScript));
	ASText joNameText = ASTextFromEncoded(JOB_OPTIONS_NAME, ASScriptToHostEncoding(kASRomanScript));

	ASCabPutText(settingsCab, XPS_CONVERSION_OPTION_KEYLANG, langText);
	ASCabPutText(settingsCab, XPS_CONVERSION_OPTION_KEY, joNameText);

	ASFileSys asFileSys = ASGetDefaultFileSys();
    ASPathName asInPathName = ASFileSysCreatePathName(NULL, ASAtomFromString("Cstring"), input, 0);
    ASPathName asOutPathName = ASFileSysCreatePathName(NULL, ASAtomFromString("Cstring"), output, 0);
	ret_val = XPS2PDFConvert(settingsCab, 0, asInPathName, NULL, &outPDDoc, NULL);
	if (ret_val)
	{
		/* DLADD: kshahn 13Aug2009 - Fix printf wording. */
        printf("file %s successfully converted\n", input);
		PDDocSave(outPDDoc, PDSaveFull, asOutPathName, ASGetDefaultFileSys(), NULL, NULL);
		PDDocClose(outPDDoc);
	}
	else
		/* DLADD: kshahn 13Aug2009 - Fix printf wording. */
        printf("file %s conversion failed\n", input);

	//clean up args when done
	ASCabDestroy(settingsCab);
	ASFileSysReleasePath(NULL, asInPathName);
	ASFileSysReleasePath(NULL, asOutPathName);
	HANDLER
		DisplayError(ERRORCODE);
	ret_val = 0;
	END_HANDLER
		return ret_val;
}
