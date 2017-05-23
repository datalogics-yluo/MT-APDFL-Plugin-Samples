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
#include <stdio.h>
#include "Utilities.h"
#include "PDCalls.h"

#ifdef MAC_PLATFORM
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#endif
//========================================================================================================
//Constructor:
//initializes APDFL and does not default the DL150PDFL.dll directory. dl150Dir should be a relative path.
//========================================================================================================
APDFLib::APDFLib(ASUns32 Flags, wchar_t* dl150Dir)
#if AIX_GCC_COMPAT
    :gccHelp()
#endif
{
    initValid = false;                            //Whether the initialization succeeded.

#ifdef WIN_PLATFORM
    if (dl150Dir == NULL)
        dl150Dir = L"..\\Binaries";               //The default DL150PDFL.lib directory.

    HINSTANCE dllInst = loadDFL150PDFL (dl150Dir);
    if (dllInst == 0)
    {
        initValid = false;
        return;
    }
#endif

    memset(&pdflData, 0, sizeof(PDFLDataRec));    //Clear the data struct so we can set its data.

    //Set PDFLDataRec's data.
    pdflData.size = sizeof(PDFLDataRec);          //Give it its size.
    pdflData.allocator = NULL;                    //Use default memory allocation procedures.
    fillDirectories();                            //Set the directory inclusion data.

    pdflData.flags = Flags;                      // Pass on initialization flags. Generally zero.

#ifdef WIN_PLATFORM
    pdflData.inst = dllInst;
#endif
    initError = PDFLInitHFT(&pdflData);           //Initialize the library.
    if (initError == 0)                           //If initError is 0, initialization succeeded.
        initValid = true;              
}

//========================================================================================================
//ASInt32 function:
//Reports whether an error happened during initialization and returns that error.
//========================================================================================================
ASInt32 APDFLib::getInitError()
{
    if (initValid == false)
    {
        std::wcerr << L"Initialization error. See \"AcroErr.h\" for more info.\n" << std::endl;
        std::wcerr << L"Error system: " << ErrGetSystem(initError) << std::endl;
        std::wcerr << L"Error Severity: " << ErrGetSeverity(initError) << std::endl;
        std::wcerr << L"Error Code: " << ErrGetCode(initError) << std::endl;
    }

    return initError;
}

//========================================================================================================
//ASInt32 function:
//Loads the DL150PDFL library dynamically.
//========================================================================================================
#ifdef WIN_PLATFORM
HINSTANCE APDFLib::loadDFL150PDFL (wchar_t* relativeDir)
{
    //Prepare to find the full path name.
    const int bufsize = 4096;                     //The size of the buffer we'll write the path to.
    TCHAR pathBuffer[bufsize] = TEXT("");         //The buffer we'll write the path to.
    TCHAR** lppPart = { NULL };                   //Recieves the address of the final name component.

    GetFullPathName (relativeDir,                 //Turn the relative path into an absolute path.
                     bufsize,
                     pathBuffer,
                     lppPart);

    SetDllDirectory(pathBuffer);                  //Add the path to the DLL directory.

    //Ensure we have read and write access to it.
    int access = _waccess(pathBuffer, 06);
    if (EACCES == access)
    {
        std::wcout << L"DL150PDFL.dll : ACCESS DENIED" << std::endl;
        return 0;
    }
    if (ENOENT == access)
    {
        std::wcout << L"DL150PDFL.dll : COULD NOT LOCATE FILE" << std::endl;
        return 0;
    }

    if (EINVAL == access)
    {
        std::wcout << L"DL150PDFL.dll : INVALID PARAMETER" << std::endl;
        return 0;
    }

    return (LoadLibrary(L"DL150PDFL.dll"));
}
#endif
#ifndef WIN_PLATFORM
size_t strnlen_safe (const char *str, size_t maxSize)
{
	if(!str)	return 0;
	size_t n;
	for (n = 0; n < maxSize && *str; n++, str++);
	return n;
}
size_t strlen_safe (const char *str)
{
	if(!str)	return 0;
	size_t n;
	for (n = 0; *str != '\0'; n++, str++);
	return n;
}
static void copyChars (char *dest, const char *src, size_t numChars)
{
	while(numChars-- > 0)
		*(dest++) = *(src++);
}
int strncpy_safe (char *dest, size_t dest_size, const char *src, size_t n)
{
	if(!dest || !src || dest_size == 0)
		return -1;

	size_t len = strnlen_safe (src, n);
	if (len < dest_size)
	{
		copyChars (dest, src, len);
		dest [len] = '\0';
		return 0;
	}
	else
	{
		copyChars (dest, src, dest_size - 1);
		dest [dest_size - 1] = '\0';
		return -1;
	}
}
int strcpy_safe (char *dest, size_t dest_size, const char *src)
{
	if(!dest || !src || dest_size == 0)
		return -1;

	return strncpy_safe (dest, dest_size, src, strlen_safe (src));
}
int strncat_safe (char *dest, size_t dest_size, const char *src, size_t n)
{
	if(!dest || !src || dest_size == 0)
		return -1;

	size_t dest_len = strnlen_safe (dest, dest_size);
	if(dest_size <= dest_len)
		return -1;

	char *new_dest = dest + dest_len;
	return strncpy_safe(new_dest, dest_size - dest_len, src, n);
}
int strcat_safe (char *dest, size_t dest_size, const char *src)
{
	if(!dest || !src || dest_size == 0)
		return -1;

	return strncat_safe (dest, dest_size, src, strlen_safe (src));
}
#endif


//========================================================================================================

//Void function:
//Sets directory information for our PDFLDataRec.
//========================================================================================================

void APDFLib::fillDirectories()
{
#ifdef WIN_PLATFORM
	//Set the font directory list and its length.
    fontDirList[0] = (ASUTF16Val*)L"..\\..\\Resources\\Font";
    fontDirList[1] = (ASUTF16Val*)L"..\\..\\Resources\\CMap";
    pdflData.dirList = fontDirList;
    pdflData.listLen = NUM_FONTS;

	//Set the color profile directory list and its length.
    colorProfDirList[0] = (ASUTF16Val*)L"..\\..\\Resources\\Color\\Profiles";
    pdflData.colorProfileDirList = colorProfDirList;
    pdflData.colorProfileDirListLen = NUM_COLOR_PROFS;

	//Set the Unicode directory.
    pdflData.cMapDirectory = fontDirList[1];
    pdflData.unicodeDirectory = (ASUTF16Val*)L"..\\..\\Resources\\Unicode";

	//Set the plugin directory and its length.
    static TCHAR pluginPathBuffer[1024];
    GetFullPathName (L"..\\Binaries", 1024, pluginPathBuffer, 0);
    pluginDirList[0] = (ASUTF16Val*)pluginPathBuffer;
    pdflData.pluginDirList = pluginDirList;
    pdflData.pluginDirListLen = NUM_PLUGIN_DIRS;
#endif
#ifdef MAC_PLATFORM
#define MAX_PATH 1000
	const unsigned int NO_OF_RESOURCE_DIR = 2;
	const char* SUB_RESOURCE_DIR[ NO_OF_RESOURCE_DIR ] = { "Font", "CMap" };

	//Set the font directory list and its length.
	fontDirList[0] = (ASUTF16Val*)"../../Resources/Font";
	fontDirList[1] = (ASUTF16Val*)"../Resources/CMap";
	pdflData.dirList = (char**)fontDirList;
	pdflData.listLen = NUM_FONTS;

	//Set the color profile directory list and its length.
	colorProfDirList[0] = (ASUTF16Val*)"../../Resources/Color/Profiles";
	pdflData.colorProfileDirList = (char**)colorProfDirList;
	pdflData.colorProfileDirListLen = NUM_COLOR_PROFS;

	//Set the Unicode directory.
	pdflData.cMapDirectory = (char*)fontDirList[1];
	pdflData.unicodeDirectory = (char*)"../../Resources/Unicode";
	char resourceDirectory[MAX_PATH];

	CFBundleRef bundleRef = CFBundleGetMainBundle();
	CFURLRef baseURL = CFBundleCopyBundleURL(bundleRef);
	CFURLRef resourceURL = CFURLCreateWithFileSystemPathRelativeToBase (kCFAllocatorDefault,
																		// DLADD: RickK 03Jan2008 - Changed all resource paths to go up one directory
																		// DLADD: further and include APDFL in the path.
																		CFSTR("../../Resources/"),
																		kCFURLPOSIXPathStyle, true, baseURL);
	CFURLGetFileSystemRepresentation (resourceURL, true, (unsigned char*)resourceDirectory, MAX_PATH);

	CFRelease(baseURL);
	CFRelease(resourceURL);

	char **tmpP = NULL;
	tmpP = (char**)malloc(sizeof(char*)*NO_OF_RESOURCE_DIR);

	char fontPath[ NO_OF_RESOURCE_DIR  ][ MAX_PATH ];
	for( int i = 0; i < NO_OF_RESOURCE_DIR; i++ ) {
		strncpy_safe(fontPath[ i ], sizeof(fontPath[ i ]), resourceDirectory, sizeof(resourceDirectory));
		strcat_safe(fontPath[ i ], sizeof(fontPath[ i ]), "/");
		strcat_safe(fontPath[ i ], sizeof(fontPath[ i ]), SUB_RESOURCE_DIR[ i ]);
	}

	for( int i = 0; i < NO_OF_RESOURCE_DIR; i++ ) {
		tmpP[i] = (char*)malloc(sizeof(char)*MAX_PATH);
		strncpy_safe(tmpP[ i ], MAX_PATH, fontPath[ i ], sizeof(fontPath[ i ]));
	}
	pluginDirList[0] = (ASUTF16Val*)tmpP;
	pdflData.pluginDirList = (char**)pluginDirList;
	pdflData.pluginDirListLen = NUM_PLUGIN_DIRS;
#endif
#ifdef UNIX_PLATFORM
#define MAX_PATH 1000

	//Set the font directory list and its length.
	fontDirList[0] = (ASUTF16Val*)"../../Resources/Font";
	fontDirList[1] = (ASUTF16Val*)"../../Resources/CMap";
	pdflData.dirList = (char**)fontDirList;
	pdflData.listLen = NUM_FONTS;

	//Set the color profile directory list and its length.
	colorProfDirList[0] = (ASUTF16Val*)"../../Resources/Color/Profiles";
	pdflData.colorProfileDirList = (char**)colorProfDirList;
	pdflData.colorProfileDirListLen = NUM_COLOR_PROFS;

	//Set the Unicode directory.
	pdflData.cMapDirectory = (char*)fontDirList[1];
	pdflData.unicodeDirectory = (char*)"../../Resources/Unicode";

	//Set the plugin
	pluginDirList[0] = (ASUTF16Val*)"../Binaries";
	pdflData.pluginDirList = (char**)pluginDirList;
    pdflData.pluginDirListLen = NUM_PLUGIN_DIRS;
#endif
}

//========================================================================================================
//Void function:
//Utility method, may be used to print APDFL errors to the terminal.
//========================================================================================================
/* static */ void APDFLib::displayError(ASErrorCode errCode)
{
    if (errCode == 0) return;

    char errStr[250];
    fprintf(stderr, "[Error 0x%08x] %s\n", errCode, ASGetErrorString(errCode, errStr, sizeof(errStr)));
}

//========================================================================================================
//Destructor:
//Terminates the library when program ends.
//========================================================================================================
APDFLib::~APDFLib()
{
    if (initValid)
        PDFLTermHFT();
}

//==============================================================================================================================
// Default Constructor - This creates a new PDDoc object. This object will be automatically freed in the APDFLDoc's destructor.
//==============================================================================================================================

APDFLDoc::APDFLDoc ()
{

    initialize ();                 //Helper method sets some of the data members to NULL values.

    DURING

        pdDoc = PDDocCreate ();    //Initialize the PDDoc data member.

    HANDLER

        printErrorHandlerMessage ();

    RERAISE ();                //Pass exception to the next HANDLER on the stack.

    END_HANDLER
}

//==============================================================================================================================
// Constructor - This constructor opens an existing PDF document. nameOfDocument is the relative path for the PDF document 
// and the bool repairDamagedFile determines whether to repair (true) or not (false) a damaged file.
//==============================================================================================================================

APDFLDoc::APDFLDoc (wchar_t * nameOfDocument, bool repairDamagedFile)
{
    CommonConstruct (nameOfDocument, repairDamagedFile);
}


//==============================================================================================================================
// Common constructor stuff.  Introduced to allow char* overload for constructor with minimal code rewriting
//==============================================================================================================================
void APDFLDoc::CommonConstruct (wchar_t* nameOfDocument, bool repairDamagedFile)
{
    initialize ();
    if (NULL == nameOfDocument)
    {
        return;
    }

    wcscpy (this->nameOfDocument, nameOfDocument);                        //Set the nameOfDocument data member.

    DURING

        setASPathName (this->nameOfDocument);                             //Set the ASPathName data member.

    pdDoc = PDDocOpen (asPathName, NULL, NULL, repairDamagedFile);    //Open the PDF document.

    ASFileSysReleasePath (NULL, asPathName);                          //Release the ASPathName that was just created.
    asPathName = NULL;

    HANDLER

        printErrorHandlerMessage ();                                       //Report any exceptions that occured.

    RERAISE ();                                                        //Pass the exception to the next handler on the stack.

    END_HANDLER
}

//==============================================================================================================================
// Constructor - This overload accepts an ordinary c-string for the file name
//==============================================================================================================================

APDFLDoc::APDFLDoc (const char * nameOfDocument, bool repairDamagedFile)
{
    if (nameOfDocument)
    {
        const size_t cSize = strlen (nameOfDocument) + 1;
        wchar_t* wc = new wchar_t[cSize];
        if (wc)
        {
            mbstowcs (wc, nameOfDocument, cSize);
        }
        CommonConstruct (wc, repairDamagedFile);
        if (wc)
        {
            delete[] wc;
        }
    }
}

//==============================================================================================================================
// initialize() - Helper method used to initialize data members to NULL values.
//==============================================================================================================================

void APDFLDoc::initialize ()
{
    pdDoc = NULL;
    asPathName = NULL;
    errorCode = 0;
    nameOfDocument[0] = L'\0';
}

//==============================================================================================================================
// saveDoc() - This overload accepts a non-wide path name, then passes through to the real function
//==============================================================================================================================

ASErrorCode APDFLDoc::saveDoc (const char* pathToSaveDoc, PDSaveFlags saveFlags)
{
    wchar_t* wc = NULL;
    if (pathToSaveDoc)
    {
        const size_t cSize = strlen (pathToSaveDoc) + 1;
        wc = new wchar_t[cSize];
        if (wc)
        {
            mbstowcs (wc, pathToSaveDoc, cSize);
        }
    }
    ASErrorCode rc = saveDoc (wc, saveFlags);
    if (wc)
    {
        delete[] wc;
    }
    return rc;
}

//==============================================================================================================================
// saveDoc() - This method saves the PDDoc. If pathToSaveDoc is supplied it will save to the location specified. If it is 
// not supplied it will overwrite the documents original location. The document will do a complete save by default, but other 
// flags may be specified.
//==============================================================================================================================

ASErrorCode APDFLDoc::saveDoc (wchar_t * pathToSaveDoc, PDSaveFlags saveFlags)
{

    DURING

        //Error checking: Ensure a name has been set before saving the document.
        if (pathToSaveDoc == NULL && nameOfDocument[0] == L'\0')
        {
            std::wcerr << L"Failed to save document ensure PDDoc has a valid name before saving. " << std::endl;
            errorCode = -1;
            return errorCode;
        }

    //Error checking: Ensure that the ASPathName has been set before saving the document.
    if (pathToSaveDoc != NULL)
        setASPathName (pathToSaveDoc);           //Use the path specified in saveDoc if it's been set.
    else
        setASPathName (nameOfDocument);          //Overwrite the original document if it hasn't been set.

    //Error Checking: Ensure document has a page before saving.
    if (PDDocGetNumPages (pdDoc) > 0)
        PDDocSave (pdDoc, saveFlags, asPathName, NULL, NULL, NULL);
    else
    {
        std::wcerr << L"Failed to save document ensure PDDoc has pages. " << std::endl;
        errorCode = -2;
    }

    ASFileSysReleasePath (NULL, asPathName);    //Release ASPathName object and set to NULL.
    asPathName = NULL;

    HANDLER

        return printErrorHandlerMessage ();         //Return the error code that was generated by the exception.

    END_HANDLER

        return errorCode;
}

//==============================================================================================================================
// setASPAthName() - Helper method used to create an ASPathName. This is called by the saveDoc and open document constructor.
//==============================================================================================================================

ASErrorCode APDFLDoc::setASPathName (wchar_t * pathToCreate)
{
    //Check to make sure there is room for path before copy
    if (wcslen (pathToCreate) <= MAX_PATH_LENGTH)
        wcscpy (this->nameOfDocument, pathToCreate);
    else
    {
        std::wcerr << L"Failed to create path, please check length of path name." << std::endl;
        return -1;
    }

    ASText textToCreatePath = NULL;         //Text object to create ASPathName

    DURING

        //Determine size of wchar_t on system and get the ASText
        if (sizeof (wchar_t) == 2)
            textToCreatePath = ASTextFromUnicode (reinterpret_cast<ASUTF16Val*> (nameOfDocument), kUTF16HostEndian);
        else
            textToCreatePath = ASTextFromUnicode (reinterpret_cast<ASUTF16Val*>(nameOfDocument), kUTF32HostEndian);

    //Create the path for output file
    asPathName = ASFileSysCreatePathFromDIPathText (NULL, textToCreatePath, NULL);

    HANDLER

        return printErrorHandlerMessage ();   //Return error code that was generated by exception 

    END_HANDLER

        ASTextDestroy (textToCreatePath);        //Release text object

    return errorCode;
}

//==============================================================================================================================
// makePath() - Wrap the various steps required to construct a proper ASPathName
//==============================================================================================================================

/* static */ ASPathName APDFLDoc::makePath (const char* path)
{
    if (!path)
    {
        return NULL;
    }
    wchar_t* wc = NULL;
    {
        const size_t cSize = strlen (path) + 1;
        wc = new wchar_t[cSize];
        if (wc)
        {
            mbstowcs (wc, path, cSize);
        }
        else
        {
            return NULL;
        }
    }
    ASPathName pn = makePath (wc);
    delete[] wc;
    return pn;
}

/* static */ ASPathName APDFLDoc::makePath (const wchar_t* path)
{
    ASText textToCreatePath = NULL;         //Text object to create ASPathName
    ASPathName pn;
    DURING
        //Determine size of wchar_t on system and get the ASText
        if (sizeof (wchar_t) == 2)
            textToCreatePath = ASTextFromUnicode (reinterpret_cast<const ASUTF16Val*> (path), kUTF16HostEndian);
        else
            textToCreatePath = ASTextFromUnicode (reinterpret_cast<const ASUTF16Val*>(path), kUTF32HostEndian);

    pn = ASFileSysCreatePathFromDIPathText (NULL, textToCreatePath, NULL);
    HANDLER
        return NULL;
    END_HANDLER
        ASTextDestroy (textToCreatePath);        //Release text object
    return pn;
}

//==============================================================================================================================
// printErrorHandlerMessage() - Helper method that reports errors and returns an error code.
//==============================================================================================================================
ASErrorCode APDFLDoc::printErrorHandlerMessage ()
{

    errorCode = ERRORCODE;                            //Get the error code that caused the exception.

    char buf[256];

    ASGetErrorString (ERRORCODE, buf, sizeof (buf));    //Get the error message that coreesponds to the error code.

    std::cerr << "Error Code: " << errorCode << "Error Message: " << buf << std::endl;

    return errorCode;
}

//==============================================================================================================================
// ~APDFLDoc() - Releases resources if they haven't already been freed.
//==============================================================================================================================

APDFLDoc::~APDFLDoc ()
{
    DURING

        if (pdDoc != NULL)                          //Close the PDDoc
            PDDocClose (pdDoc);

    if (asPathName != NULL)                     //Close the pathname
        ASFileSysReleasePath (NULL, asPathName);

    HANDLER

        printErrorHandlerMessage ();

    RERAISE ();                                  //Pass exception to the next handler on the stack.

    END_HANDLER
}
