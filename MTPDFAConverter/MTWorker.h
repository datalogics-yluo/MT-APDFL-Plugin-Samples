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
#ifndef _MTWorker_h_
#define _MTWorker_h_
#include  "MyPDFLibUtils.h"
#include "SDKThreads.h"

/** The main entry point for the worker thread. Synchronises with the WatchFolder to 
	get files to process.
 */
ThreadFuncReturnType DoWork(ThreadArgs *pArgs);


#endif //_MTWorker_h_
