CC=gcc
CFLAGS=-Wall -g -std=c99
CLMACROS=-DATI_OS_LINUX
CLINCLUDES=-I$$AMDAPPSDKROOT/include
CLLIB=-lOpenCL
OBJDIR = obj
BUILDDIR = bin
CLLIBDIR=-L$$AMDAPPSDKROOT/lib/x86_64
 
all: mkdirs MyDeviceQuery kernelInfo bitstuff.o clutils.o
	
MyDeviceQuery: MyDeviceQuery.o clutils.o
	$(CC) $(CFLAGS) $(CLMACROS) $(patsubst %,$(OBJDIR)/%,$^) $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $(BUILDDIR)/$@
	
MyDeviceQuery.o: MyDeviceQuery.c
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
	
kernelInfo: kernelInfo.o
	$(CC) $(CFLAGS) $(CLMACROS) $(patsubst %,$(OBJDIR)/%,$^) $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $(BUILDDIR)/$@
	
kernelInfo.o: kernelInfo.c
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
	
clutils.o: clutils.c clutils.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@	

bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

.PHONY: clean mkdirs

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*

