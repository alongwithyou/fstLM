CXXFLAGS+=-I../../kaldi/tools/openfst/include
LDFLAGS+=-L../../kaldi/tools/openfst/lib

all:


TESTFILES =

OBJFILES =

BINFILES = compute-ppl

$(BINFILES): $(OBJFILES)


all: $(TESTFILES) $(BINFILES)

clean:
	-rm *.o $(BINFILES) $(TESTFILES)

CXXFLAGS += -Wall -g
# suppressing warnings from openfst
CXXFLAGS+=-Wno-sign-compare -Wno-unused-local-typedefs
LDFLAGS += -g -lfst

ifeq ($(shell uname), Darwin)
 CXXFLAGS += -stdlib=libstdc++
 LDFLAGS += -stdlib=libstdc++
endif

LDLIBS =
CXX = g++
CC = $(CXX)
RANLIB = ranlib
AR = ar

depend:
	-$(CXX) -M $(CXXFLAGS) *.cc > .depend.mk

.depend.mk: depend
-include .depend.mk

-include .flags.mk
