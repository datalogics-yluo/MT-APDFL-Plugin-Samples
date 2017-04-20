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
#ifndef _FlattenPDFWorker_h_
#define _FlattenPDFWorker_h_
#include  "MyPDFLibUtils.h"
#include "SDKThreads.h"

/** The main entry point for the worker thread. Synchronises with the WatchFolder to 
	get files to process. Get the input file and set up the output file stream. Calls 
	ExtractDocTect to  do the text extraction.
	@params pArgs IN thread arguments. See WatchFolder.h for details.
	@return thread exit value
	@see ASPathName
	@see ASFileSysOpenFile
	@see ASFileSysReleasePath
	@see ASFileSysPathFromDIPath
	@see ASRegisterErrorString
	@see ASRaise
	@see ASFileClose
	@see PDDocClose
	@see ASGetErrorString
*/
ThreadFuncReturnType FlattenPDF(ThreadArgs *pArgs);


#endif //_FlattenPDFWorker_h_
