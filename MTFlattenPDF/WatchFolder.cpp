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
#include "MyPDFLibUtils.h"
#include "WatchFolder.h"
#include "stdio.h"

// sleep in windows is in ms, on unix it is in seconds.
#ifdef WINDOWS
#define base 1000
#else
#define base 1
#endif

WatchFolder::WatchFolder(ASPathName folderToWatch, const char * outDir, ASInt32 numFiles) : numToReturn(numFiles){
	// initialise the mutex used to protect our vectors
	InitCS(this->watchFolderMutex);
	
	this->folder = ASFileSysCopyPath(NULL, folderToWatch);
	this->outputFolder = outDir;
	ASFileSysItemPropsRec folderProps;
	folderProps.size = sizeof(ASFileSysItemPropsRec);
	ASErrorCode err = ASFileSysGetItemProps(NULL,this->folder,&folderProps);
	memcpy(&(this->folderModDate),&folderProps.modDate,sizeof(ASTimeRec));
	ASFileSysItemPropsRec fileProps;
	fileProps.size = sizeof(ASFileSysItemPropsRec);

	// we allocate an ASPathName for each path found. We always allocate before 
	// using the folder iterator, so we have to free the last one created.
	// Our vector takes ownership of the ASPathNames
	ASPathName *newPath = static_cast<ASPathName *>(ASmalloc(sizeof(ASPathName)));
	// We have the folder, we build up the list of files.
	ASFolderIterator folderIter = NULL;
	folderIter = ASFileSysFirstFolderItem(NULL,this->folder, &fileProps, newPath);
	ASBool iterate = true;
	if (folderIter == NULL){
		iterate = false;
	}
	while (iterate){
		// we only add the file if it is a pdf
		char* diPath = ASFileSysDIPathFromPath(NULL,*newPath, NULL);
		string pathToTest(diPath);
		ASfree(diPath);
		if (pathToTest.rfind(".pdf") == pathToTest.length()-4){
			this->addFile(*newPath);
		}	
		else
			ASFileSysReleasePath(NULL, *newPath);
		iterate = ASFileSysNextFolderItem(NULL,folderIter, &fileProps, newPath);
	}
	if (folderIter != NULL)
		ASFileSysDestroyFolderIterator(NULL,folderIter);
	ASfree(newPath);

}

WatchFolder::~WatchFolder(){
	// clean out the vectors

	while(dirContents.size()>0){
		char* tmpStr = dirContents.back();
		dirContents.pop_back();
		ASfree(tmpStr);
	}
	while(doneFiles.size()>0){
		char* tmpStr = doneFiles.back();
		doneFiles.pop_back();
		ASfree(tmpStr);
	}	
	ASFileSysReleasePath( NULL, folder );
	DestroyCS(this->watchFolderMutex);	
}	


void 
WatchFolder::watchFolder(){
	ASFileSysItemPropsRec folderProps;
	folderProps.size = sizeof(ASFileSysItemPropsRec);
	EnterCS(this->watchFolderMutex);
	ASInt32 oldNum = this->numToReturn;
	LeaveCS(this->watchFolderMutex);
	while (1){
		printf("Scanning for input files...\n");
		ASErrorCode err = ASFileSysGetItemProps(NULL,this->folder,&folderProps);
		while (memcmp(&folderProps.modDate,&(this->folderModDate),sizeof(ASTimeRec))==0){
			// this while loop represents the main idle loop of the primary thread.
			// we can test to see if enough files have been converted, if so we can 
			// simply exit the method.
			EnterCS(this->watchFolderMutex);
			if (this->numToReturn == 0){
				LeaveCS(this->watchFolderMutex);
				printf("File limit reached, terminating\n");
				return;
			}
			LeaveCS(this->watchFolderMutex);

			Sleep(10*base);
			err = ASFileSysGetItemProps(NULL,this->folder,&folderProps);
			if ((err != 0) || folderProps.isThere == false){
				fprintf(stderr,"Error getting the folder properties : WatchFolder.cpp:103\n");
			}
			EnterCS(this->watchFolderMutex);
			if (this->numToReturn == 0)
				return;
			ASInt32 newNum = this->numToReturn;
			LeaveCS(this->watchFolderMutex);
			if (newNum!=oldNum){
				printf("%d files to go\n",newNum);
				oldNum=newNum;
			}
		}
		// there must have been an update on the folder...
		// look at each file, if we haven't seen it before, add it to our list.
		ASFileSysItemPropsRec fileProps;
		fileProps.size = sizeof(ASFileSysItemPropsRec);
		ASInt32 size = sizeof(ASPathName);
		ASPathName *newPath = static_cast<ASPathName *>(ASmalloc(size));
		// We have the folder, we build up the list of files.
		ASFolderIterator folderIter = ASFileSysFirstFolderItem(NULL,this->folder, &fileProps, newPath);
		char* buff = ASFileSysDisplayStringFromPath(NULL,*newPath);
		ASBool iterate = true;
		if (folderIter == NULL){
			iterate = false;
		}

		while (iterate){
			// we only add the file if it is a pdf
		char* diPath = ASFileSysDIPathFromPath(NULL,*newPath, NULL);
		string pathToTest(diPath);
		ASfree(diPath);
			if (pathToTest.rfind(".pdf") == pathToTest.length()-4) {
				this->addFile(*newPath);
			}
			else
				ASFileSysReleasePath(NULL, *newPath);
			iterate = ASFileSysNextFolderItem(NULL,folderIter, &fileProps, newPath);
		}
		memcpy(&(this->folderModDate),&folderProps.modDate,sizeof(ASTimeRec));
		ASFileSysDestroyFolderIterator(NULL,folderIter);
		// Similar to the constructor, we only need to free the unused instance of newPath
		ASfree(newPath);
	}
}

void 
WatchFolder::addFile(ASPathName newPath){
	// A robust implementation would make sure the file is not being written to here
	// This sample leaves this as an exercise for the reader
	char * pathToTest = ASFileSysDIPathFromPath(NULL,newPath, NULL);
	ASBool fileExists = false;
	EnterCS(this->watchFolderMutex);
	for (unsigned int tmp=0;tmp<doneFiles.size();tmp++){
		char* tmpStr = doneFiles[tmp];
		fileExists = (strcmp(pathToTest,tmpStr) == 0);
		if (fileExists){
			break;
		}
	}
	LeaveCS(this->watchFolderMutex);
	if (fileExists == false){
		// we need to test the todo list, we have to do this within the 
		// critical section in case it is removed as we test it.
		// A better implementation might allow for a shrinking todo list.
		EnterCS(this->watchFolderMutex);
		// make sure it isn't on the todo list either.
		for (unsigned int tmp2=0;tmp2<dirContents.size();tmp2++){
			char* tmpStr = dirContents[tmp2];
			fileExists = (strcmp(pathToTest,tmpStr) == 0);
			if (fileExists){
				break;
			}
		}
		LeaveCS(this->watchFolderMutex);
		if (fileExists == false){
			// debug:printf("Adding new file %s\n",ASFileSysDIPathFromPath(NULL,newPath,NULL));
			EnterCS(this->watchFolderMutex);
			dirContents.push_back(pathToTest);
			LeaveCS(this->watchFolderMutex);
		}
	}
	ASFileSysReleasePath(NULL, newPath);
}

const char * WatchFolder::getOutputDir()
{
	return this->outputFolder;
}


const char * 
WatchFolder::getFile(){
	// the newPath == NULL indicating one of our worker threads is looking for work
	EnterCS(this->watchFolderMutex);
	if (this->numToReturn == 0){
		// we do not need to convert any more files, returning NULL to the worker thread
		// indicates it should terminate.
		LeaveCS(this->watchFolderMutex);
		return NULL;
	}
	while (dirContents.size() == 0){
		LeaveCS(this->watchFolderMutex);
		Sleep(1*base);
		EnterCS(this->watchFolderMutex);
		if (this->numToReturn == 0){
			LeaveCS(this->watchFolderMutex);
			break;
		}
	}
	// we get here, we know we have wokrk to be done and we are inthe mutex.
	// we need to test that as well as having files to be processed, we still
	// need to process them (i.e. some other thread might have taken the number
	// of files we have to process down to 0 while we were waiting for work.
	if (this->numToReturn == 0){
		// leave the critical section and return null, this tells the worker 
		// thread to terminate.
		LeaveCS(this->watchFolderMutex);
		return NULL;
	}
	char * tmp = dirContents.back();
	dirContents.pop_back();
	
	// done files needs to be updated
	doneFiles.push_back(tmp);
	
	// update the number of files still to be handled
	this->numToReturn--;
	LeaveCS(this->watchFolderMutex);
	return tmp;
}


