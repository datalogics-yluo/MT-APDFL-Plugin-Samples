SAMPNAME = MTFlattenPDF
OTHER_OBJS = $(SAMPNAME).o WatchFolder.o FlattenPDFWorker.o

include ../utils/common.mak

CXXFLAGS += -DLOAD_PLUGIN -DLINUX
EXTRA_LIBS = -lDL150PDFPort

$(SAMPNAME).o : $(SRC)/MTFlattenPDF.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

WatchFolder.o : $(SRC)/WatchFolder.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

FlattenPDFWorker.o : $(SRC)/FlattenPDFWorker.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

