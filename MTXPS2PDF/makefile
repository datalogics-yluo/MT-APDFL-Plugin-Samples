SAMPNAME = MTXPS2PDF
OTHER_OBJS = $(SAMPNAME).o WatchFolder.o XPS2PDFWorker.o

include ../utils/common.mak

CXXFLAGS += -DLOAD_PLUGIN -DLINUX
EXTRA_LIBS = -lDL150PDFPort

$(SAMPNAME).o : $(SRC)/MTXPS2PDF.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

WatchFolder.o : $(SRC)/WatchFolder.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

XPS2PDFWorker.o : $(SRC)/XPS2PDFWorker.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

