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


#include "WatchFolder.h"
#include "MTWorker.h"
#include "stdio.h"
#ifndef WIN_PLATFORM
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <iterator>
using namespace std;
#ifdef WIN_PLATFORM
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
 #endif
#ifdef MAC_PLATFORM
#include "macUtils.h"
#endif

/* From the Utils directory */
#include  "MyPDFLibUtils.h"
#include "SDKThreads.h"

// DLADD: wfles 17Jul2007 - Utilize central main() function from MyPDFLibApp.cpp
// DLADD: and MainProc with function arguments.  MyPDFLInit and MyPDFLTerm now occur
// DLADD: in the centralized main() function.
int MainProc(int argc, char *argv[] )
{
	INIT_AUTO_POOL(autoReleasePool);	/* Required only on MAC platform */
	
	ThreadInfo * myThreads = NULL;
	ThreadArgs * myThreadArgs = NULL;
	char *folderName = NULL;
	char *outdir = NULL;


/* DLADD */
#ifdef WIN_ENV
	int pattr = 0;  // dummy variable
#else
	pthread_attr_t *pattr = NULL;
#endif
	ASInt32 numFiles = 20;
	ASInt32 numThreads = 20;
	if (argc > 5) {
		printf( "Usage: %s folderPath outdir [numfiles [numthreads]]\n", argv[0] );
		printf("The folder path must be absolute\n");
		return 0;
	}	
	folderName = "indir";
	outdir = "outdir";

	printf("Watched folder defined as %s\n",folderName);
	printf("output folder defined as %s\n", outdir);
	printf("Will process %d PDF files\n",numFiles);
	printf("Will create %d threads\n",numThreads);
	ASInt32 loop = numFiles;
	if (numFiles < numThreads)
		loop = numThreads;
	char firstFile[256], otherFile[256];    

	
#ifdef WIN_PLATFORM
	strcpy_safe(firstFile, "input.pdf");
	for(ASInt32 index = 0; index < loop; index++)
	{
		//Copy Input.pdf to Input-[index].pdf
		strcpy_safe(otherFile, folderName);
		strcat_safe(otherFile, "\\input");
		char str[50];
		sprintf_safe(str, "-%d.pdf", index);
		strcat_safe(otherFile, str);
		BOOL b = CopyFile(firstFile, otherFile, 0);
	}

#else //Linux/Mac/Unix
	strcpy(firstFile, "input.pdf");
	for(int index = 0; index < loop; index++)
	{
		//Copy Input.pdf to Input-[index].pdf
		strcpy(otherFile, folderName);
		strcat(otherFile, "/input");
		char str[50];
		sprintf(str, "-%d.pdf", index);
		strcat(otherFile, str);
		
		std::ofstream file(otherFile);
		if(file){
			//Clean up previous exist file
			if( remove( otherFile ) != 0 )
			{
				printf("error in clean up %s\n", otherFile);
				return -1;
			}
		}
		
		int fd_to, fd_from;
		char buf[4096];
		ssize_t nread;
		int saved_errno;
		fd_from = open(firstFile, O_RDONLY);
		if (fd_from < 0)
		{
			printf("error in open original file\n");
			return -1;
		}
		fd_to = open(otherFile, O_WRONLY | O_CREAT | O_EXCL, 0666);
		if (fd_to < 0)
		{
			printf("error in copy input file\n");
			goto out_error;
		}
		while (nread = read(fd_from, buf, sizeof buf), nread > 0)
		{
			char *out_ptr = buf;
			ssize_t nwritten;

			do {
				nwritten = write(fd_to, out_ptr, nread);

				if (nwritten >= 0)
				{
					nread -= nwritten;
					out_ptr += nwritten;
				}
				else if (errno != EINTR)
				{
					goto out_error;
				}
			} while (nread > 0);
		}
	
		if (nread == 0)
		{
			if (close(fd_to) < 0)
			{
				fd_to = -1;
				goto out_error;
			}
			close(fd_from);
			/* Success! */
		}

out_error:
		saved_errno = errno;

		close(fd_from);
		if (fd_to >= 0)
			close(fd_to);
	}
#endif //WIN_PLATFORM

	//create the ASPathName
#ifdef MAC_PLATFORM
	ASPathName folderToWatch = GetMacPath(folderName);
#else
	ASPathName folderToWatch = ASFileSysCreatePathName(NULL,ASAtomFromString("Cstring"),folderName,NULL);
#endif
	ASFileSysItemPropsRec folderProps;
	folderProps.size = sizeof(ASFileSysItemPropsRec);
	ASErrorCode err = ASFileSysGetItemProps(NULL,folderToWatch,&folderProps);
	if ((err != 0) || (folderProps.isThere == false) || (folderProps.type != kASFileSysFolder)){
		printf("Watch Folder missing.\n");
			return -1;
	}
	
	// creating the watched folder object initialises the state of the application.
	// After the watched folder object is created, other threads can synchronise with it.
	WatchFolder * myWF = new WatchFolder(folderToWatch, outdir, numFiles);
	ASFileSysReleasePath(NULL, folderToWatch);

	// Allocate the thread block
	myThreads = static_cast<ThreadInfo *>(ASmalloc( sizeof( ThreadInfo ) * numThreads));

	// Allocate a ThreadArgs object for each thread
	myThreadArgs = static_cast<ThreadArgs *>(ASmalloc( sizeof( ThreadArgs ) * numThreads));

/* DLADD */
#ifndef WIN32
 #if defined(RS6000AIX)
/*
 * The default stack size for threads on AIX is 96K. This can
 * be not enough for applications, which are more complex than
 * this sample. The code below demonstrates how can the threads'
 * stack size be customized on UNIX-like platforms.
 * (The default stack size on Linux is 1024K.)
 */
	pthread_attr_t attr;
	int ok;
	pattr = &attr;
	size_t stackSize = 128*1024*numThreads;

	// Initialize 'attr' with default values.
	ok = pthread_attr_init(&attr);
	if (ok != 0)
	{
		printf( "pthread_attr_init(...) error.\n" );
		return -1;
	}
	// Customize the stack size.
	ok = pthread_attr_setstacksize(&attr, stackSize);
	if (ok != 0)
	{
		printf( "MainProc: pthread_attr_setstacksize(...) error.\n" );
		return -1;
	}
 #endif
#endif

	// Start the threads, passing their thread number to each
	for (ASInt32 i = 0; i < numThreads; i++) {
		char *buff = static_cast<char *>(ASmalloc(40));
		sprintf_safe(buff, 40, "MTTextExtract thread %d", i);
		myThreadArgs[i].tName = buff;

		myThreadArgs[i].watchFolder = myWF;

//		if (!createThread( GetWords, &myThreadArgs[i], myThreads[i] )){
		if (!createThread2( DoWork, &myThreadArgs[i], myThreads[i], pattr )){	// DLADD
			printf( "Thread creation %d failed\n", i );
		}
	}

	myWF->watchFolder();
	// DLADD: RickK 07May2009 APDFL9 - Print normal progress messages to
	// DLADD: "stdout" to avoid mixing with error messages, or implying
	// DLADD: that this is an error message.
	fprintf(stdout,"Waiting for worker thread completion\n");
	// Clean up the threads, after waiting for each to exit.
	for (ASInt32 j = 0; j < numThreads; j++) {
		waitThread(myThreads[j]);
		printf("Thread %s joined\n",myThreadArgs[j].tName);
		destroyThread(myThreads[j]);
		ASfree(myThreadArgs[j].tName);
	}

/* DLADD */
#ifndef WIN32
 #if defined(RS6000AIX)
	ok = pthread_attr_destroy(&attr);
 #endif
#endif

	ASfree(myThreads);
	ASfree(myThreadArgs);
	delete myWF;
	
	RELEASE_AUTO_POOL(autoReleasePool);	/* Required only on MAC platform */
	
	return 0;
}

// DLADD: wfles 17Jul2007 - Utilize central main() function from MyPDFLibApp.cpp
// DLADD: and MainProc with function arguments.
#define INCLUDE_MYPDFLIBAPP_CPP 1
#include "MyPDFLibApp.cpp"
#undef INCLUDE_MYPDFLIBAPP_CPP
