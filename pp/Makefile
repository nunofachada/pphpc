CC = gcc
OBJDIR = ../obj
BUILDDIR = ../bin
UTILSDIR = ../utils
CFLAGS = -Wall -g -std=gnu99 `pkg-config --cflags glib-2.0`
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64 
UTILOBJS = $(OBJDIR)/clutils.o $(OBJDIR)/bitstuff.o $(OBJDIR)/Profiler.o
TESTSDIR = tests

all: utils mkdirs PredPreyGPUSort PredPreyCPU PredPreyGPU 

profiling: CLMACROS += -DCLPROFILER
profiling: all

PredPreyGPUSort: PredPreyGPUSort.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyGPUSort.o: PredPreyGPUSort.c PredPreyGPUSort.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -o $(OBJDIR)/$@ -c $<

.PHONY: clean mkdirs utils

utils:
	cd $(UTILSDIR); make

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
	cd $(UTILSDIR); make clean
