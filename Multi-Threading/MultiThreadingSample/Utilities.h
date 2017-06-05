// Copyright (c) 2015-2016, Datalogics, Inc. All rights reserved.
//
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
//========================================================================
// Sample - Initialize: This class defines an object used for the
// initialization and termination of the Adobe PDF Library. It will also
// report initialization errors.
//
// InitializeLibrary.cpp: Contains the method implementations.
// InitializeLibrary.h: Contains the class definition.
//========================================================================

#ifndef UTILITIES_H
#define UTILITIES_H

#define NUM_FONTS 2          //The number of font directories we'll include during initialization.    
#define NUM_COLOR_PROFS 1    //The number of color profile directories we'll include during initialization.
#define NUM_PLUGIN_DIRS 1    //The number of plugin directories we'll include during initialization.

#include <io.h>
#include <iostream>
#include <cstring>
#include <vector>
#include "PDCalls.h"
#include "PDFLCalls.h"
#include "ASCalls.h"
#include "ASExtraCalls.h"
#include "MTHeader.h"


#ifdef AIX_GCC_COMPAT
#include <dlfcn.h>
// DLADD RobB 31Jan2017 - RAII helper to explicitly initialize xlC libraries on AIX from a g++ main()
class GCCAIXHelper
{
 public:
    GCCAIXHelper()
        :hAXE(NULL), hXMP(NULL), hJP2K(NULL), hBIBUt(NULL), hBIB(NULL),
        hACE(NULL), hARE(NULL), hAGM(NULL), hCT(NULL), hPDFL(NULL)
        {
            if (!(hAXE = dlopen("libDL150AXE8SharedExpat.so", RTLD_NOW | RTLD_GLOBAL)))
                if (!(hXMP = dlopen("libDL150AdobeXMP.so", RTLD_NOW | RTLD_GLOBAL)))
                    if (!(hJP2K = dlopen("libDL150JP2K.so", RTLD_NOW | RTLD_GLOBAL)))
                        if (!(hBIBUt = dlopen("libDL150BIBUtils.so", RTLD_NOW | RTLD_GLOBAL)))
                            if (!(hBIB = dlopen("libDL150BIB.so", RTLD_NOW | RTLD_GLOBAL)))
                                if (!(hACE = dlopen("libDL150ACE.so", RTLD_NOW | RTLD_GLOBAL)))
                                    if (!(hARE = dlopen("libDL150ARE.so", RTLD_NOW | RTLD_GLOBAL)))
                                        if (!(hAGM = dlopen("libDL150AGM.so", RTLD_NOW | RTLD_GLOBAL)))
                                            if (!(hCT = dlopen("libDL150CoolType.so", RTLD_NOW | RTLD_GLOBAL)))
                                                hPDFL = dlopen("libDL150pdfl.so", RTLD_NOW | RTLD_GLOBAL);
        }
    ~GCCAIXHelper()
        {
            if (hPDFL)
                dlclose(hPDFL);
            if (hCT)
                dlclose(hCT);
            if (hAGM)
                dlclose(hAGM);
            if (hARE)
                dlclose(hARE);
            if (hACE)
                dlclose(hACE);
            if (hBIB)
                dlclose(hBIB);
            if (hBIBUt)
                dlclose(hBIBUt);
            if (hJP2K)
                dlclose(hJP2K);
            if (hXMP)
                dlclose(hXMP);
            if (hAXE)
                dlclose(hAXE);
        }
 private:
     void *hAXE, *hXMP, *hJP2K, *hBIBUt, *hBIB, *hACE, *hARE, *hAGM, *hCT, *hPDFL;
};
#endif // AIX_GCC_COMPAT

/* We may specify in the commands which memory manager to use. this section defines the legal managers
*/
typedef enum memoryMangers
{
    no_memoryManager,
    malloc_memoryManager,
    tcmalloc_memory_Manager,
    rpmalloc_memory_manager,
    NumberOfMemManager
} MemoryManagers;

#include "no_memory.h"
#include "malloc_memory.h"
#include "tcmalloc_memory.h"
#include "rpmalloc_memory.h"


TKAllocatorProcs *StringToMemManager (char *name);

class APDFLib
{
public:
    //Constructor initializes APDFL Using choices encoded in the attributes.
    APDFLib (ASUns32 flags, attributes *FrameAttributes);
    ~APDFLib();                                       //Destructor terminates APDFL.

    ASInt32 getInitError();                           //Reports whether an error happened during initialization and returns that error.
    ASBool isValid() { return initValid; };           //Returns true if the library initialized successfully.
    static void displayError(ASErrorCode);            //Utility method, may be used to print APDFL errors to the terminal.

private:
    PDFLDataRec pdflData;                             //A struct containing information that APDFL initializes with.
    ASInt32 initError;                                //Used to record initialization errors.
    ASBool initValid;                                 //Set to true if the library initializes successfully.
    TKAllocatorProcsP memoryAllocator;                //Set to te memory allocator to use


    void fillDirectories(attributes *frameAttributes);                           //Sets directory information for our PDFLDataRec.
#ifdef WIN_PLATFORM
    HINSTANCE loadDL150PDFL(char* relativeDir);   //Loads the DL150PDFL library dynamically.
#endif

    char** fontDirList;
    char** colorProfDirList;
    char** pluginDirList;
#if AIX_GCC_COMPAT
    GCCAIXHelper gccHelp;
#endif

#ifdef WIN_PLATFORM
    char *ToUTF16AndAppendToStringPool (char *string);
#endif

    char BinariesPath[2048];
    char PluginsPath[2048];
    char ResourcesPath[2048];
    char *stringPool;
    size_t stringPoolSize;
};

PDDoc OpenSampleFile (char *);
void  SaveDocument (PDDoc doc, char *name, PDSaveFlags saveFlags = (PDSaveFull | PDSaveCollectGarbage));
ASPathName GetMacPath (char * filename);


#endif //UTILITIES_H
