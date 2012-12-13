CC = gcc
CFLAGS = -Wall -g -std=gnu99
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
BUILDDIR = bin
UTILOBJS = utils/ocl/clerrors.o utils/ocl/fileutils.o utils/ocl/bitstuff.o utils/ocl/clinfo.o
 
all: PredPreyGPUSort PredPreyGPU PredPreyCPU cleanobjs

profiler: CLMACROS += -DCLPROFILER
profiler: all
	
PredPreyGPUSort: PredPreyGPUSort.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o PredPreyGPUProfiler.o PredPreyGPUEvents.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)

PredPreyGPUSort.o: PredPreyGPUSort.c PredPreyGPUSort.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@
	
PredPreyGPUProfiler.o: PredPreyGPUProfiler.c PredPreyGPUProfiler.h PredPreyGPUEvents.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyGPUEvents.o: PredPreyGPUEvents.c PredPreyGPUEvents.h PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -o $@ -c $<

clean: cleanobjs
	rm -f $(BUILDDIR)/*
	
cleanobjs:
	rm -f *.o
