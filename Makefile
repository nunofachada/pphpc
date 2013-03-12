CC = gcc
CFLAGS = -Wall -g -std=gnu99
CFLAGS_GLIB = `pkg-config --cflags glib-2.0`
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
LFLAGS_GLIB = `pkg-config --libs glib-2.0`
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
OBJDIR = obj
BUILDDIR = bin
UTILSDIR = utils
UTILOBJS = $(UTILSDIR)/$(OBJDIR)/clutils.o $(UTILSDIR)/$(OBJDIR)/bitstuff.o
TESTSDIR = tests

all: makeutils mkdirs Tests PredPreyGPUSort PredPreyCPU PredPreyGPU 

profiling: CLMACROS += -DCLPROFILER
profiling: all

PredPreyGPUSort: PredPreyGPUSort.o PredPreyCommon.o Profiler.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(LFLAGS_GLIB)
	
PredPreyGPU: PredPreyGPU.o PredPreyCommon.o Profiler.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(LFLAGS_GLIB)

PredPreyCPU: PredPreyCPU.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(LFLAGS_GLIB)

PredPreyGPUSort.o: PredPreyGPUSort.c PredPreyGPUSort.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyGPU.o: PredPreyGPU.c PredPreyGPU.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

PredPreyCPU.o: PredPreyCPU.c PredPreyCPU.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@
	
Profiler.o: Profiler.c Profiler.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(LFLAGS_GLIB) -o $(OBJDIR)/$@

PredPreyCommon.o: PredPreyCommon.c PredPreyCommon.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) $(CLINCLUDES) -o $(OBJDIR)/$@ -c $<

Tests: test_profiler

test_profiler: test_profiler.o PredPreyCommon.o Profiler.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(LFLAGS_GLIB)
	
test_profiler.o: $(TESTSDIR)/test_profiler.c 
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@


.PHONY: clean mkdirs makeutils

makeutils:
	cd $(UTILSDIR); make

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
