SAMPNAME = MTPDFAConverter
OTHER_OBJS = $(SAMPNAME).o WatchFolder.o MTWorker.o

include ../utils/common.mak

CXXFLAGS += -DLOAD_PLUGIN -DLINUX
EXTRA_LIBS = -lDL150PDFPort

$(SAMPNAME).o : $(SRC)/MTmain.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

WatchFolder.o : $(SRC)/WatchFolder.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

MTWorker.o : $(SRC)/MTWorker.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c $< -o $@

