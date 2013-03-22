CC=gcc
CFLAGS=-Wall -g -std=c99 `pkg-config --cflags glib-2.0`
CLMACROS=-DATI_OS_LINUX
CLINCLUDES=-I$$AMDAPPSDKROOT/include
CLLIB=-lOpenCL
OBJDIR = ../obj
BUILDDIR = ../bin
CLLIBDIR=-L$$AMDAPPSDKROOT/lib/x86_64
LFLAGS  = `pkg-config --libs glib-2.0`
TESTSDIR = tests

all: mkdirs MyDeviceQuery kernelInfo bitstuff.o clutils.o Profiler.o Tests

.PHONY: all Test clean mkdirs

MyDeviceQuery: MyDeviceQuery.o clutils.o
	$(CC) $(CFLAGS) $(CLMACROS) $(patsubst %,$(OBJDIR)/%,$^) $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(LFLAGS)
	
MyDeviceQuery.o: MyDeviceQuery.c
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
	
kernelInfo: kernelInfo.o
	$(CC) $(CFLAGS) $(CLMACROS) $(patsubst %,$(OBJDIR)/%,$^) $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $(BUILDDIR)/$@
	
kernelInfo.o: kernelInfo.c
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
	
clutils.o: clutils.c clutils.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@	

Profiler.o: Profiler.c Profiler.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@ 

bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@
	
Tests: test_profiler test_utils

test_profiler: test_profiler.o Profiler.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(CLLIB)
	
test_profiler.o: $(TESTSDIR)/test_profiler.c 
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@

test_utils: test_utils.o clutils.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS) $(CLLIB)
	
test_utils.o: $(TESTSDIR)/test_utils.c 
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) -o $(OBJDIR)/$@	

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*

