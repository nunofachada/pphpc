CC = gcc
CFLAGS = -Wall -g -std=gnu99
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
OBJDIR = obj
BUILDDIR = bin
UTILSDIR = utils
UTILOBJS = $(UTILSDIR)/$(OBJDIR)/clerrors.o $(UTILSDIR)/$(OBJDIR)/fileutils.o $(UTILSDIR)/$(OBJDIR)/bitstuff.o $(UTILSDIR)/$(OBJDIR)/clinfo.o
 
all: makeutils mkdirs Test PredPreyGPUSort PredPreyGPU PredPreyCPU

profiler: CLMACROS += -DCLPROFILER
profiler: all

PredPreyGPUSort: PredPreyGPUSort.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o PredPreyGPUProfiler.o PredPreyGPUEvents.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

PredPreyGPUSort.o: PredPreyGPUSort.c PredPreyGPUSort.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@
	
PredPreyGPUProfiler.o: PredPreyGPUProfiler.c PredPreyGPUProfiler.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyGPUEvents.o: PredPreyGPUEvents.c PredPreyGPUEvents.h PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -o $(OBJDIR)/$@ -c $<

Test.o: Test.c
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

Test: Test.o PredPreyCommon.o PredPreyGPUProfiler.o PredPreyGPUEvents.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

.PHONY: clean mkdirs makeutils

makeutils:
	cd $(UTILSDIR); make

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
