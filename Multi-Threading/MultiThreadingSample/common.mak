.NOTPARALLEL:	dummy

ifeq ($(OS), )
$(error Need to set the OS environment variable)
endif

# 26Aug2009 - Give each sample a copy of the common modules
# since plugin samples compile these differently
COMMON_OBJS = PDFLInitCommon.o PDFLInitHFT.o Utilities.o \
			  Flattener_Worker.o NonAPDFL_Worker.o PDFA_Worker.o \
			  PDFX_Worker.o Rasterizer_Worker.o \
			  TextExtract_Worker.o Worker.o XPS2PDF_Worker.o

INCLUDE = ../Include/Headers
SOURCE= ../Include/Source


include $(OS).mak

include paths.rel

default: $(SAMPNAME)

CPPFLAGS = -I. -I$(INCLUDE) -I$(DLI_INCLUDE) -I$(COMMON)
CFLAGS = $(CCFLAGS)

$(SAMPNAME) : $(COMMON_OBJS) $(OTHER_OBJS)
	$(CXX) -o $@ $(COMMON_OBJS) $(OTHER_OBJS) $(LDFLAGS) $(LIBS) $(EXTRA_LIBS)

##
# The files have the source and object in different directories, explicit rules
# are needed. 
#
# And, the '.c' files have to be compiled using the C++ compiler.
##

PDFLInitCommon.o : $(SOURCE)/PDFLInitCommon.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

PDFLInitHFT.o : $(SOURCE)/PDFLInitHFT.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

Utilities.o : Utilities.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

Flattener_Worker.o : Flattener_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

NonAPDFL_Worker.o : NonAPDFL_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

PDFA_Worker.o : PDFA_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

PDFX_Worker.o : PDFX_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

Rasterizer_Worker.o : Rasterizer_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	
TextExtract_Worker.o : TextExtract_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

XPS2PDF_Worker.o : XPS2PDF_Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

Worker.o : Worker.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) *.o core out.* $(SAMPNAME) 

