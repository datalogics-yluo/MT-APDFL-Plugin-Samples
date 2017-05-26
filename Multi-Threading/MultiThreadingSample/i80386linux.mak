CC = /usr/bin/gcc
CXX = /usr/bin/g++

ifeq ($(STAGE), debug)
    DEBUG=-DDEBUG -D_DEBUG
else
    DEBUG=-DNDEBUG
endif

ifeq ($(BUILD_64_BIT), true)
    ARCH_FLAGS = -m64
else
    ARCH_FLAGS = -m32
endif

CCFLAGS  = $(ARCH_FLAGS) -g $(PDF_FDIR_DEF) -DNO_PRAGMA_ONCE -DUNIX_PLATFORM=1 -DUNIX_ENV=1 -DPRODUCT=\"HFTLibrary.h\" $(DEBUG) -D_REENTRANT -Wno-multichar -DPDFL_SDK_SAMPLE -DPI_ACROCOLOR_VERSION=AcroColorHFT_VERSION_6 -DTOOLKIT

CXXFLAGS = $(CCFLAGS)

LDFLAGS = $(ARCH_FLAGS) -L$(PDFL_PATH)
LIBS = -lDL150pdfl -lDL150CoolType -lDL150AGM -lDL150BIB -lDL150ACE -lDL150ARE \
	   -lDL150BIBUtils -lDL150JP2K -lDL150AdobeXMP -lDL150AXE8SharedExpat \
	   -licucnv -licudata -lpthread
