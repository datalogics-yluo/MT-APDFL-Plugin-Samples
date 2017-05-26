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
#if WIN_PLATFORM
#include <io.h>
#else
#include <sys/uio.h>
#endif
#include <iostream>
#include <cstring>
#include <vector>
#include "PDCalls.h"
#include "PDFLCalls.h"
#include "ASCalls.h"
#include "ASExtraCalls.h"


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
class APDFLib
{
public:
    //Constructor initializes APDFL and sets the path to DL150PDFL.dll to dl150Dir. If NULL is passed, defaults to ../../../Binaries. dl150Dir should be a relative path.
    APDFLib (ASUns32 flags = 0, char* binariesPath = NULL, char *pluginsPath = NULL, char *resourcesPath = NULL);
    ~APDFLib();                                       //Destructor terminates APDFL.

    ASInt32 getInitError();                           //Reports whether an error happened during initialization and returns that error.
    ASBool isValid() { return initValid; };           //Returns true if the library initialized successfully.
    static void displayError(ASErrorCode);            //Utility method, may be used to print APDFL errors to the terminal.

private:
    APDFLib(const APDFLib&);                          // not implemented
    PDFLDataRec pdflData;                             //A struct containing information that APDFL initializes with.
    ASInt32 initError;                                //Used to record initialization errors.
    ASBool initValid;                                 //Set to true if the library initializes successfully.

    void fillDirectories();                           //Sets directory information for our PDFLDataRec.
#if WIN_PLATFORM
    HINSTANCE loadDL150PDFL(char* relativeDir);   //Loads the DL150PDFL library dynamically.
#endif

    ASUTF16Val* fontDirList[NUM_FONTS];               //List of font directories we'll include during initialization.              //TODO: platform divergences
    ASUTF16Val* colorProfDirList[NUM_COLOR_PROFS];    //List of color profile directories we'll include during initialization.     //TODO: platform divergences
    ASUTF16Val* pluginDirList[NUM_PLUGIN_DIRS];       //List of plugin directories we'll include during initialization.            //TODO: platform divergences
#if AIX_GCC_COMPAT
    GCCAIXHelper gccHelp;
#endif


    char BinariesPath[2048];
    char PluginsPath[2048];
    char ResourcesPath[2048];
};

class APDFLDoc
{

private:

    static const unsigned MAX_PATH_LENGTH = 4096;                          //Private data members assosciated with the document.
    wchar_t nameOfDocument[MAX_PATH_LENGTH];

    volatile ASPathName asPathName;
    ASErrorCode errorCode;

    void initialize ();                                                     //Called in constructor to initialize some data members.
    ASErrorCode setASPathName (wchar_t*);                                  //Helper method used to create ASPathName objects for operations.
    void CommonConstruct (wchar_t*, bool);                                   //Helper to allow char* overload of constructor with minimal code copying

    APDFLDoc (const APDFLDoc&);                                             //Do not allow copy constructor or assignment operator to be used.
    APDFLDoc& operator=(const APDFLDoc&);                                  //in order to prevent shallow copies of objects.

public:

    volatile PDDoc pdDoc;                                                  //Made public so it can be accessed directly.

    APDFLDoc (wchar_t*, bool doRepairDamagedFile);                          //Constructor used to open a document.
    APDFLDoc (const char*, bool doRepairDamagedFile);                       //Constructor used to open a document.
    APDFLDoc ();                                                            //Constructor used to create a document.

    ASSize_t numPages () { return (PDDocGetNumPages (pdDoc)); }

    volatile PDDoc& getPDDoc () { return pdDoc; };                           //Returns a reference to the PDDoc that was created or opened.

    ASErrorCode saveDoc (wchar_t* = NULL, PDSaveFlags = PDSaveFull);        //Used to save the document, may be provided a path and PDSaveFlags.
    ASErrorCode saveDoc (const char*, PDSaveFlags = PDSaveFull);            //Used to save the document to a specified non-wide string, may be provided PDSaveFlags.

    ~APDFLDoc ();                                                           //Destructor frees up resources.

    static ASPathName makePath (const char* path);                         //Provide functionality for device independent path construction
    static ASPathName makePath (const wchar_t* path);

    ASErrorCode printErrorHandlerMessage ();

    static ASUnicodeFormat GetHostUnicodeFormat () { return (sizeof (wchar_t) == 2 ? kUTF16HostEndian : kUTF32HostEndian); }
};

#endif //UTILITIES_H
