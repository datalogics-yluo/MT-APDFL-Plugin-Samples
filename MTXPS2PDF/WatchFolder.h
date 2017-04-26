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
#ifndef _WatchFolder_h_
#define _WatchFolder_h_


#include <vector>
#include <string>
#include <string.h>

#include "ASCalls.h"
#include "PDCalls.h"
#include "SDKThreads.h"
class WatchFolder;

typedef struct ThreadArgs {
	char *tName;
	WatchFolder * watchFolder;
} ThreadArgs;

/* Line endings - lineend is defined so that it is correct for Mac, Win, UNIX */
#ifdef WIN_ENV
#define lineend "\r\n"
#elif MAC_ENV
#define lineend "\r"
#else
#define lineend "\n"
#endif

using namespace std;

/** Implements a simple watched folder using  the PDFL APIs.
	The folder to watch is defined in the constructor. The 
	abstraction tests for updates to the folder. If the folder
	has been updated it adds the updated file to an internal list.

	Folder "update" is based on filename, if you overwrite an existing 
	file in the folder, the new file is not dealt with. The "termination"
	implementation is unsurprisingly naive, it is based on converting a 
	preset number of files. Once this limit has been reached, the watched
	folder will return NULL to parties requesting work (even if there is
	work available).
*/
class WatchFolder {
public:
	/** ctor. creates an instance of a watched folder using the path
		specified in the parameter. This is a simple implementation, 
		it detects changes to the folder and updates the files to deal
		with based on filename only. If you overwrite a file in the
		watched folder, it is not dealt with. 
		@param folderToWatch IN the folder we want to watch, relative to 
				the current (application) directory
		@param numFiles IN the number of files to deal with. 

		@see ASFileSysCreatePathName
		@see ASFileSysItemPropsRec
		@see ASFileSysGetItemProps
		@see ASRaise
		@see ASFileSysFirstFolderItem
		@see ASFileSysDIPathFromPath
		@see ASFileSysNextFolderItem
		@see ASFileSysDestroyFolderIterator
	*/
	WatchFolder(ASPathName folderToWatch,const char * outdir, ASInt32 numFiles);
	/** dtor */
	virtual ~WatchFolder();
	/** The synch point for the worker threads. These threads will block within
		this call until there is work (files) available.
		@return the next path in the list. If there is no path present, the
			thread will block. The return value is a device independent 
			representation of the path, ownership of the char * is retained by
			the watched folder. Returns NULL to indicate the watchedFolder is
			terminating.
	*/
	const char * getFile();

	const char * getOutputDir();


	/** This method is the guardian for the folder. It represents the main
		event loop for the thread that watches the folder. It records the 
		time of updates to the folder and handles any changes made to the 
		folder.
		@see ASFileSysItemPropsRec
		@see ASFileSysGetItemProps
		@see ASFileSysFirstFolderItem
		@see ASFileSysDisplayStringFromPath
		@see ASFileSysDIPathFromPath
		@see ASFileSysNextFolderItem
		@see ASFileSysDestroyFolderIterator
	*/
	void watchFolder();

private:
	/** Adds work (new files). Called with a path of a file to be processed. This 
		call does not guarantee the file will be processed, if the limit of files
		is reached before this file is processed it will not be touched.
		@param newPath IN the path of the file to be added.
		@see ASFileSysDIPathFromPath
	*/
	void addFile(ASPathName newPath);
	/** We cache the folder modification date. This lets us determine which files
		have been updated when a change on the folder has occured.
	*/
	ASTimeRec folderModDate;
	/** The watched folder */
	ASPathName folder;
	/** This represents the files still to be dealt with.
	*/
	vector<char *> dirContents;
	/** This is a naive watched folder, we don't bother going to the file properties to discover
		new or updated files, we just maintain a list of files we have dealt with.
		If a file is added that is not already in the list, we process it.
	*/
	vector<char *> doneFiles;
	/** The watchFolder method is the main synchronisation point. It is where all the
		threads meet, the thread that continues to add files to the watched folder
		list and whatever threads exist to take them off (the worker threads).
		We use a mutex to protect the data-structure. 
	*/
	CSMutex watchFolderMutex;

	/** the name representing the watched folder. It is relative to the executable.
	*/
	string folderName;

	const char * outputFolder;

public:
	/** The number of files to return to the calling client. Once this limit is reached, NULL is	
		returned.
	*/
	ASInt32 numToReturn;
};

#endif 


