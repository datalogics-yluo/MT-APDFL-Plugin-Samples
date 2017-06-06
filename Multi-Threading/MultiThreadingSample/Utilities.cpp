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
//  Initializes the APDFL library:
//  responds to the attributes:
//   APDFLPath                      Where to find the libraries, default is ../Binaries
//   ResourcesPath                  Where to find resources, default is ../../Resources
//...PluginPath                     Where to find plugins, default is APDFLPath.
//   ColorsPath                     Where to find color profiles, defaults to ..\\..\\Resources\\Color\\Profiles
//...UnicodePath                    Where to find the Unicode Directory, defaults to ../../Resources/Unicode
//   CMapsPath                      Where to find the CMaps Directory, defaults to ../../Resources/CMaps
//   MemoryManger                   Which memory manager should APDFL Use?
//========================================================================================================
APDFLib::APDFLib(ASUns32 Flags, attributes *FrameAttributes)
#if AIX_GCC_COMPAT
    :gccHelp()
#endif
{
    initValid = false;                            //Whether the initialization succeeded.
    stringPool = NULL;
    stringPoolSize = 0;

    if ((FrameAttributes != NULL) && (FrameAttributes->IsKeyPresent ("APDFLPath")))
    {
        char *binaries = FrameAttributes->GetKeyValue ("APDFLPath")->value(0);
        strcpy (BinariesPath, binaries);
        free (binaries);
    }
    else
#ifdef WIN_PLATFORM
        strcpy (BinariesPath, "..\\Binaries");
#else
        strcpy (BinariesPath, "../Binaries");
#endif

#ifdef WIN_PLATFORM
    HINSTANCE dllInst = loadDL150PDFL (BinariesPath);
    if (dllInst == 0)
    {
        initValid = false;
        return;
    }
#endif

    memset(&pdflData, 0, sizeof(PDFLDataRec));    //Clear the data struct so we can set its data.

    //Set PDFLDataRec's data.
    pdflData.size = sizeof(PDFLDataRec);          //Give it its size.
    fillDirectories(FrameAttributes);             //Set the directory inclusion data.

    pdflData.flags = Flags;                      // Pass on initialization flags. Generally zero.

    if ((FrameAttributes != NULL) && (FrameAttributes->IsKeyPresent ("MemoryManager")))
        pdflData.allocator = StringToMemManager (FrameAttributes->GetKeyValue ("MemoryManager")->value (0), &managerID);
    else
        pdflData.allocator = NULL;

    InitializeMemoryManager ();

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
HINSTANCE APDFLib::loadDL150PDFL (char* binarysDir)
{

    SetDllDirectoryA (binarysDir);                  //Add the path to the DLL directory.

    //Ensure we have read and write access to it.
    int found = _access (binarysDir, 06);
    if (EACCES == found)
    {
        std::wcout << L"DL150PDFL.dll : ACCESS DENIED" << std::endl;
        return 0;
    }
    if (ENOENT == found)
    {
        std::wcout << L"DL150PDFL.dll : COULD NOT LOCATE FILE" << std::endl;
        return 0;
    }

    if (EINVAL == found)
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

#ifdef WIN_PLATFORM
char *APDFLib::ToUTF16AndAppendToStringPool (char *string)
{ 
    char fullPath[4096];
    GetFullPathNameA (string, 4096, fullPath, 0);

    size_t length = strlen (fullPath);
    size_t save = stringPoolSize;
    stringPoolSize += (length * 2) + 2;
    stringPool = (char *)realloc (stringPool, stringPoolSize);
    char *ptr = stringPool + save;
    mbstowcs ((wchar_t *)ptr, fullPath, (length * 2) + 2);
    return ((char *)save);
}
#else
char *APDFLib::AppendToStringPool (char *string)
{
    char *fullPath;
    fullpath = realpath (string, NULL);

    size_t length = strlen (fullPath);
    size_t save = stringPoolSize;
    stringPoolSize += (length) + 1;
    stringPool = (char *)realloc (stringPool, stringPoolSize);
    char *ptr = stringPool + save;
    strcpy ((char *)ptr, fullPath);
    free (fullPath);
    return ((char *)save);
}
#endif

//========================================================================================================

//Void function:
//Sets directory information for our PDFLDataRec.
//========================================================================================================

void APDFLib::fillDirectories (attributes *frameAttributes)
{
    ASBool resourcesSupplied = frameAttributes->IsKeyPresent ("ResourcesPath");
    ASBool unicodeSupplied = frameAttributes->IsKeyPresent ("UnicodePath");
    ASBool pluginsSupplied = frameAttributes->IsKeyPresent ("PluginsPath");
    ASBool colorsSupplied = frameAttributes->IsKeyPresent ("ColorsPath");
    ASBool cmapsSupplied = frameAttributes->IsKeyPresent ("CMapsPath");

#ifdef WIN_PLATFORM

    //Set the font directory list and its length.
    if (resourcesSupplied)
    {
        valuelist *resources = frameAttributes->GetKeyValue ("ResourcesPath");
        pdflData.listLen = resources->size ();
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        for (int index = 0; index < pdflData.listLen; index++)
            fontDirList[index] = ToUTF16AndAppendToStringPool (resources->value (index));
    }
    else
    {

        pdflData.listLen = 2;
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        fontDirList[0] = ToUTF16AndAppendToStringPool ("..\\..\\Resources\\Font");
        fontDirList[1] = ToUTF16AndAppendToStringPool ("..\\..\\Resources\\CMap");

    }
    pdflData.dirList = (ASUTF16Val **)fontDirList;

    //Set the color profile directory list and its length.
    if (colorsSupplied)
    {
        valuelist *colors = frameAttributes->GetKeyValue ("ColorsPath");
        pdflData.colorProfileDirListLen = colors->size ();
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        for (int index = 0; index < pdflData.colorProfileDirListLen; index++)
            colorProfDirList[index] = ToUTF16AndAppendToStringPool (colors->value (index));
    }
    else
    {
        pdflData.colorProfileDirListLen = 1;
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        colorProfDirList[0] = ToUTF16AndAppendToStringPool ("..\\..\\Resources\\Color\\Profiles");
    }
    pdflData.colorProfileDirList = (ASUTF16Val **)colorProfDirList;

    //Set the Unicode directory.
    if (unicodeSupplied)
        pdflData.unicodeDirectory = (ASUTF16Val *)ToUTF16AndAppendToStringPool (frameAttributes->GetKeyValue ("UnicodePath")->value (0));
    else
        pdflData.unicodeDirectory = (ASUTF16Val *)ToUTF16AndAppendToStringPool ("..\\..\\Resources\\Unicode");

    /* Set the CMaps directory */
    if (cmapsSupplied)
        pdflData.cMapDirectory = (ASUTF16Val *)ToUTF16AndAppendToStringPool (frameAttributes->GetKeyValue ("CMapsPath")->value (0));
    else
        pdflData.cMapDirectory = (ASUTF16Val *)ToUTF16AndAppendToStringPool ("../../Resources/CMap");

    //Set the plugin directory and its length.
    if (pluginsSupplied)
    {
        valuelist *plugins = frameAttributes->GetKeyValue ("PluginsPath");
        pdflData.pluginDirListLen = plugins->size ();
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        for (int index = 0; index < pdflData.pluginDirListLen; index++)
            pluginDirList[index] = ToUTF16AndAppendToStringPool (plugins->value (index));
    }
    else
    {
        pdflData.pluginDirListLen = 1;
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        pluginDirList[0] = ToUTF16AndAppendToStringPool ("..\\Binaries");
    }
    pdflData.pluginDirList = (ASUTF16Val **)pluginDirList;

#endif

#ifdef MAC_PLATFORM
#define MAX_PATH 1000
    const unsigned int NO_OF_RESOURCE_DIR = 2;
    const char* SUB_RESOURCE_DIR[ NO_OF_RESOURCE_DIR ] = { "Font", "CMap" };

    //Set the font directory list and its length.
    if (resourcesSupplied)
    {
        valuelist *resources = frameAttributes->GetKeyValue ("ResourcesPath");
        pdflData.listLen = resources->size ();
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        for (int index = 0; index < pdflData.listLen; index++)
            fontDirList[index] = AppendToStringPool (resources->value (index));
    }
    else
    {
        pdflData.listLen = 2;
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        fontDirList[0] = AppendToStringPool("../../Resources/Font");
        fontDirList[1] = AppendToStringPool("../Resources/CMap");

    }
    pdflData.dirList = (char**)fontDirList;

    //Set the color profile directory list and its length.
    if (colorsSupplied)
    {
        valuelist *colors = frameAttributes->GetKeyValue ("ColorsPath");
        pdflData.colorProfileDirListLen = colors->size ();
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        for (int index = 0; index < pdflData.colorProfileDirListLen; index++)
            colorProfDirList[index] = AppendToStringPool(colors->value (index));
    }
    else
    {
        pdflData.colorProfileDirListLen = 1;
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        colorProfDirList[0] = AppendToStringPool("../../Resources/Color/Profiles")
    }
    pdflData.colorProfileDirList = colorProfDirList;

    //Set the Unicode directory.
    if (unicodeSupplied)
        pdflData.unicodeDirectory = AppendToStringPool(frameAttributes->GetKeyValue ("UnicodePath")->value (0));
    else
        pdflData.unicodeDirectory = AppendToStringPool("../../Resources/Unicode");

    /* Set the CMaps directory */
    if (cmapsSupplied)
        pdflData.cMapDirectory = AppendToStringPool(frameAttributes->GetKeyValue ("CMapsPath")->value (0));
    else
        pdflData.cMapDirectory = AppendToStringPool("../../Resources/CMap");


    /* I do notknow what this piece of code is SUPPOSED todo. 
    ** It looks like it is setting the plugin path, butit looks like it sets it "interestingly
    **
    ** For now, I am going to turn it off
    */
#if 0
    char resourceDirectory[MAX_PATH];

    char resource

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
#else
    //Set the plugin
    if (pluginsSupplied)
    {
        valuelist *plugins = frameAttributes->GetKeyValue ("PluginsPath");
        pdflData.pluginDirListLen = plugins->size ();
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        for (int index = 0; index < pdflData.pluginDirListLen; index++)
            pluginDirList[index] = AppendToStringPool (plugins->value (index));
    }
    else
    {
        pdflData.pluginDirListLen = 1;
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        pluginDirList[0] = AppendToStringPool ("../Binaries");
    }
    pdflData.pluginDirList = (char**)pluginDirList;
#endif

#endif

#ifdef UNIX_PLATFORM
#define MAX_PATH 1000

    //Set the font directory list and its length.
    if (resourcesSupplied)
    {
        valuelist *resources = frameAttributes->GetKeyValue ("ResourcesPath");
        pdflData.listLen = resources->size ();
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        for (int index = 0; index < pdflData.listLen; index++)
            fontDirList[index] = AppendToStringPool(resources->value (index));
    }
    else
    {
        pdflData.listLen = 2;
        fontDirList = (char **)malloc (sizeof (char *) * pdflData.listLen);
        fontDirList[0] = AppendToStringPool("../../Resources/Font");
        fontDirList[1] = AppendToStringPool("../../Resources/CMap");

    }
    pdflData.dirList = (char**)fontDirList;

    //Set the color profile directory list and its length.
    if (colorsSupplied)
    {
        valuelist *colors = frameAttributes->GetKeyValue ("ColorsPath");
        pdflData.colorProfileDirListLen = colors->size ();
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        for (int index = 0; index < pdflData.colorProfileDirListLen; index++)
            colorProfDirList[index] = AppendToStringPool(colors->value (index));
    }
    else
    {
        pdflData.colorProfileDirListLen = 1;
        colorProfDirList = (char **)malloc (sizeof (char *) * pdflData.colorProfileDirListLen);
        colorProfDirList[0] = AppendToStringPool("../../Resources/Color/Profiles");
    }
    pdflData.colorProfileDirList = (char**)colorProfDirList;

    //Set the Unicode directory.
    if (unicodeSupplied)
        pdflData.unicodeDirectory = AppendToStringPool(frameAttributes->GetKeyValue ("UnicodePath")->value (0));
    else
        pdflData.unicodeDirectory = AppendToStringPool("../../Resources/Unicode");

    /* Set the CMaps directory */
    if (cmapsSupplied)
        pdflData.cMapDirectory = AppendToStringPool((frameAttributes->GetKeyValue ("CMapsPath")->value (0));
    else
        pdflData.cMapDirectory = AppendToStringPool("../../Resources/CMap");

    //Set the plugin
    if (pluginsSupplied)
    {
        valuelist *plugins = frameAttributes->GetKeyValue ("PluginsPath");
        pdflData.pluginDirListLen = plugins->size ();
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        for (int index = 0; index < pdflData.pluginDirListLen; index++)
            pluginDirList[index] = AppendToStringPool (plugins->value (index));
    }
    else
    {
        pdflData.pluginDirListLen = 1;
        pluginDirList = (char **)malloc (sizeof (char *) * pdflData.pluginDirListLen);
        pluginDirList[0] = AppendToStringPool ("../Binaries");
    }
    pdflData.pluginDirList = (char**)pluginDirList;

#endif



    /* All of the "addressses" above are actually displacements into the stringPool
    **
    ** Once the string pool is complete, we need to resolve them into addresses
    */
    for (int index = 0; index < pdflData.listLen; index++)
        fontDirList[index] = stringPool + (size_t)fontDirList[index];
    for (int index = 0; index < pdflData.colorProfileDirListLen; index++)
        colorProfDirList[index] = stringPool + (size_t)colorProfDirList[index];
    for (int index = 0; index < pdflData.pluginDirListLen; index++)
        pluginDirList[index] = stringPool + (size_t)pluginDirList[index];
    pdflData.cMapDirectory = (ASUTF16Val *)(stringPool + (size_t)pdflData.cMapDirectory);
    pdflData.unicodeDirectory = (ASUTF16Val *)(stringPool + (size_t)pdflData.unicodeDirectory);
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
    if (stringPool != NULL)
        free (stringPool);
    free (fontDirList);
    free (colorProfDirList);
    free (pluginDirList);
    FinalizeMemoryManager ();
}


PDDoc OpenSampleFile (char *name)
{
        ASPathName pathName;
#if MAC_PLATFORM
        pathName = GetMacPath (fileName);
#else
        pathName = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), name, 0);
#endif

    PDDoc doc = PDDocOpen (pathName, NULL, NULL, true);

    ASFileSysReleasePath (NULL, pathName);                          //Release the ASPathName that was just created.

    return (doc);
}


void SaveDocument (PDDoc doc,char *pathName, PDSaveFlags saveFlags)
{

    ASPathName path;
#if MAC_PLATFORM
    path = GetMacPath (pathName);
#else
    path = ASFileSysCreatePathName (NULL, ASAtomFromString ("Cstring"), pathName, 0);
#endif

    PDDocSave (doc, saveFlags, path, NULL, NULL, NULL);

    ASFileSysReleasePath (NULL, path);    //Release ASPathName object and set to NULL.
}

#ifdef MAC_PLATFORM
ASPathName GetMacPath (char * filename)
{
    /* This function returns an ASPathName for opening and closing PDF files on the Macintosh platform.
    Paths can be specified with a relative Unix path and an ASPathName will be created relative
    to the directory from which the application was launched. The ASPathName returned must be freed with
    ASFileSysReleasePath(ASGetDefaultFileSys(),path);

    *****
    The code in this function is unsupported.
    It is a simple utility for the sample applications for handling paths without the need for a dialog.
    *****
    */
    ASPathName newPathName = NULL;
    ASPathName appPathName = NULL;

    //relative? Then relative to app
    if (filename[0] != '/')
    {
#if TARGET_RT_MAC_MACHO
        /* Refs:
        * Finding your application's directory: http://developer.apple.com/qa/fl/fl14.html
        * Locating Application Support Files Under Mac OS X: http://developer.apple.com/technotes/tn/tn2015.html
        */
        OSErr err;
        ProcessSerialNumber psn;

        err = GetCurrentProcess (&psn);
        if (err != noErr) return NULL;

        FSRef location;
        err = GetProcessBundleLocation (&psn, &location);
        if (err != noErr) return NULL;

        FSRef parent;
        err = FSGetCatalogInfo (&location, 0, NULL, NULL, NULL, &parent);
        if (err != noErr) return NULL;

        appPathName = ASFileSysCreatePathName (NULL, ASAtomFromString ("FSRef"), &parent, NULL);
#else
        //On the mac we need an absolute path, so we get the location of the application
        CFBundleRef bundleRef = CFBundleGetMainBundle ();
        CFURLRef bundleURL = CFBundleCopyExecutableURL (bundleRef);
        ASPathName pathName = ASFileSysCreatePathName (NULL, ASAtomFromString ("CFURLRef"), bundleURL, NULL);
        CFRelease (bundleURL);

        //bounce out of the package to the app container folder
        appPathName = ASFileSysPathFromDIPath (NULL, "../../..", pathName);
        ASFileSysReleasePath (ASGetDefaultFileSys (), pathName);
#endif
        newPathName = ASFileSysCreatePathFromDIPath (NULL, filename, appPathName);
    }
    else
        newPathName = ASFileSysCreatePathName (NULL,
        ASAtomFromString ("POSIXPath"), filename, NULL);

    if (appPathName)
        ASFileSysReleasePath (ASGetDefaultFileSys (), appPathName);



    return newPathName;
}


#endif


/* This is the controller for selecting a memory manager
** 
** To add a new memory manager, add it's ID to the MemoryManagers enum,
** and it's include file to the list of memory manager includes, in utilities.h, 
** Add it's external name to memManagerNames, and it's access
** routine to the switch in StringToMemManager, below.
**
** Add a new pair of files. The .h file needs to define it's access routine only. 
** the .cpp file sould include defintions for allocation, reallocation, deallocation,
** an remaining size.
*/

char *memManagerNames[NumberOfMemManager] =
{ "NONE", "MALLOC", "TCMALLOC", "RPMALLOC" };


TKAllocatorProcs *StringToMemManager (char *name, MemoryManagers *saveId)
{
    char localName[20];
    strcpy (localName, name);
    for (int index = 0; localName[index] != 0; index++)
        localName[index] = toupper (localName[index]);

    MemoryManagers id = NumberOfMemManager;
    for (int index = 0; index < NumberOfMemManager; index++)
    {
        if (!strcmp (localName, memManagerNames[index]))
        {
            id = (MemoryManagers)index;
            break;
        }
    }

    *saveId = id;

    switch (id)
    {
    case no_memoryManager:
        return (nomemory_access ());
        break;

    case malloc_memoryManager:
        return (malloc_access ());
        break;

    case tcmalloc_memory_Manager:
        return (tcmalloc_access ());
        break;

    case rpmalloc_memory_manager:
        return (rpmalloc_access ());
        break;

    case NumberOfMemManager:
    default:
        return (NULL);
        break;

    }

    return (NULL);
}

void APDFLib::InitializeMemoryManager ()
{
    switch (managerID)
    {
        case rpmalloc_memory_manager:
            rpmalloc_thread_initialize ();
            break;

        default:
            break;
    }
}

void APDFLib::FinalizeMemoryManager ()
{
    switch (managerID)
    {
    case rpmalloc_memory_manager:
        rpmalloc_thread_finalize ();
        break;

    default:
        break;
    }
}
