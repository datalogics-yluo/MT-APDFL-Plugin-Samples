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
#include <time.h>
#include "MTWorker.h"
#include "WatchFolder.h"
#include "stdio.h"
#include "ASCalls.h"
#include "CosCalls.h"
#include "PDCalls.h"
#include "PDFProcessorCalls.h"

ThreadFuncReturnType DoWork(ThreadArgs *pArgs)
{
	INIT_AUTO_POOL(autoReleasePool);	/* Required only on MAC platform */
	
	WatchFolder *theWF = pArgs->watchFolder;

	// we intitialise outside the loop.
	MyPDFLInit();
	gPDFProcessorHFT = InitPDFProcessorHFT;
	if (!PDFProcessorInitialize())
	{
		fprintf(stderr, "Error initializing PDF/A Processorr -- aborting\n");
		return NULL;
	}
	clock_t start_time[2], stop_time[2]; //pcg
	double duration;//pcg
	ASAtom atStructTreeRoot = ASAtomFromString("StructTreeRoot");
	ASAtom atMarkInfo = ASAtomFromString("MarkInfo");
	ASAtom atMarked = ASAtomFromString("Marked");


	while (1){
		// first thing we do is get the path from the WatchFolder

		char* fileToProcess;
		if(theWF->numToReturn > 0)
			fileToProcess = (char*) theWF->getFile();
		else 
			fileToProcess = NULL;

		// we terminate the main loop if the watchFolder abstraction
		// gives us a NULL token. This thread can now die.
		if (fileToProcess == NULL) {
			PDFProcessorTerminate();
			MyPDFLTerm();
			/////break;
			return 0;
		}

		// we have a valid file to extract the text from.
		// the destination file is filenmae.txt, in the parent directory.
		// this code is intentionally simple, a real implementation would
		// have a robust implementation.
		string tmpstr(fileToProcess);
		ASSize_t sentinal = tmpstr.rfind('/');
		string filename = tmpstr.substr(sentinal,string::npos);
		//filename += ".pdf";
		string outfile(theWF->getOutputDir());
		outfile += filename;
		
		// At this point we now have a path with the parent's directory stripped off.
		// Create the output file.
		ASPathName outPath = ASFileSysCreatePathFromDIPath(NULL, outfile.c_str(), NULL);

		volatile PDDoc docP = NULL;     // document we will be extracting text from
		DURING
			volatile ASPathName filePath = NULL;
			DURING
				 filePath = ASFileSysPathFromDIPath(NULL,fileToProcess, NULL);
			HANDLER
				ASRaise(ASRegisterErrorString(ErrAlways,"Cannot get DI path"));
			END_HANDLER;
			// Open the PDF file. If cannot be opened, raise
			if ((docP = PDDocOpen(filePath,NULL,NULL,true))==NULL) {
				char buffer[400];
				sprintf_safe(buffer,sizeof(buffer),"%s cannot open %s",pArgs->tName,fileToProcess);
				ASRaise(ASRegisterErrorString(ErrAlways,buffer));
			}
			ASFileSysReleasePath( NULL, filePath );

			CosDoc cosPDFDoc = PDDocGetCosDoc(docP);
			CosObj DocRoot = CosDocGetRoot(cosPDFDoc);
			ASBool bMarked = false;
			ASBool bStructTree = CosObjGetType(CosDictGet(DocRoot, atStructTreeRoot)) == CosDict;

			CosObj cosMarkInfo = CosDictGet(DocRoot, atMarkInfo);
			if (CosObjGetType(cosMarkInfo) == CosDict)
			{
				CosObj cosMarked = CosDictGet(cosMarkInfo, atMarked);
				if (CosObjGetType(cosMarked) == CosBoolean)
					bMarked = CosBooleanValue(cosMarked);
			}
			ASBool bTagged = (bMarked && bStructTree);


			PDFProcessorPDFAConvertParamsRec userParams;
			ASInt32 res = false;

			memset(&userParams, 0, sizeof(PDFProcessorPDFAConvertParamsRec));
			userParams.size = sizeof(PDFProcessorPDFAConvertParamsRec);
			userParams.noRasterizationOnFontErrors = false;
			userParams.removeAllAnnotations = false;
			
			start_time[0] = clock();
			res = PDFProcessorConvertAndSaveToPDFA(docP, outPath, ASGetDefaultFileSys(), bTagged ? kPDFProcessorConvertToPDFA1aRGB : kPDFProcessorConvertToPDFA1bRGB, &userParams);
			stop_time[0] = clock();
			duration = ((double)(stop_time[0] - start_time[0]) / CLOCKS_PER_SEC);
			if (res)
				printf("File %s saved as PDF/A: %2.2fs\n", filename.c_str(), duration);
			else
				printf("Conversion of file %s has failed...\n", filename.c_str());

		HANDLER
			char buf[200];
			ASGetErrorString(ERRORCODE, buf, sizeof(buf));
			fprintf(stderr, "Error 0x%lx: (%s) %s\n", ERRORCODE, filename.c_str(), buf);
		END_HANDLER

		// finish up
		if (outPath != NULL){
				ASFileSysReleasePath(NULL, outPath);
				outPath = NULL;
			}
			if (docP != NULL){
				PDDocClose(docP);
				docP = NULL;
			}


			//PDFProcessorTerminate();
			//MyPDFLTerm();
	}
	
	RELEASE_AUTO_POOL(autoReleasePool); /* Required only on MAC platform */
	
	return 0;
}
