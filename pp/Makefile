# Check if required directories are defined
ifndef OBJDIR
OBJDIR := ../obj
$(warning OBJDIR is not defined, assuming default $(OBJDIR))
endif
ifndef BUILDDIR
BUILDDIR := ../bin
$(warning BUILDDIR is not defined, assuming default $(BUILDDIR))
endif
ifndef UTILSINCLUDEDIR
UTILSINCLUDEDIR := ../cf4ocl
$(warning UTILSINCLUDEDIR is not defined, assuming default $(UTILSINCLUDEDIR))
endif

# Define variables
CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -std=c99 `pkg-config --cflags glib-2.0`
CLMACROS += -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
UTILSINCLUDES = -I$(UTILSINCLUDEDIR)
LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64 
UTILOBJS = $(OBJDIR)/clutils.o $(OBJDIR)/clprofiler.o
TESTSDIR = tests

# Phony targets
.PHONY: all clean mkdirs profiling

# Targets and rules
all: mkdirs PredPreyGPUSort PredPreyCPU PredPreyGPU 

profiling: CLMACROS += -DCLPROFILER
profiling: all

PredPreyGPUSort: PredPreyGPUSort.o PredPreyCommon.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyGPUSort.o: PredPreyGPUSort.c PredPreyGPUSort.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h $(UTILSINCLUDEDIR)/gerrorf.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@ -c $<
	
bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@	

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
	cd $(UTILSDIR); make clean
