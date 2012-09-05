CC = gcc
CFLAGS = -Wall -g -std=gnu99
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
BUILDDIR = bin
UTILOBJS = utils/clerrors.o utils/fileutils.o utils/bitstuff.o utils/clinfo.o
 
all: PredPreyGPU PredPreyCPU cleanobjs

profiler: CLMACROS += -DCLPROFILER
profiler: all
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -o $@ -c $<

clean: cleanobjs
	rm -f $(BUILDDIR)/*
	
cleanobjs:
	rm -f *.o
