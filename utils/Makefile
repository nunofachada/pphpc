CC=gcc
CFLAGS=-Wall -g -std=c99
CLMACROS=-DATI_OS_LINUX
CLINCLUDES=-I$$AMDAPPSDKROOT/include
CLLIB=-lOpenCL
OBJDIR = obj
BUILDDIR = bin
CLLIBDIR=-L$$AMDAPPSDKROOT/lib/x86_64
 
all: mkdirs MyDeviceQuery clerrors.o fileutils.o bitstuff.o clinfo.o
	
MyDeviceQuery: MyDeviceQuery.o clinfo.o
	$(CC) $(CFLAGS) $(CLMACROS) $(patsubst %,$(OBJDIR)/%,$^) $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $(BUILDDIR)/$@
	
MyDeviceQuery.o: MyDeviceQuery.c
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
	
clerrors.o: clerrors.c clerrors.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@	

clinfo.o: clinfo.c clinfo.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@	

fileutils.o: fileutils.c fileutils.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

.PHONY: clean mkdirs

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*

