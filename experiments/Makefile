CC = gcc
CFLAGS = -Wall -g -std=gnu99
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
BUILDDIR = bin
UTILOBJS = ../utils/ocl/clerrors.o ../utils/ocl/fileutils.o ../utils/ocl/bitstuff.o ../utils/ocl/clinfo.o
 
Matmult: Matmult.c PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $^ $(UTILOBJS) $(LFLAGS)

PredPreyCommon.o: ../PredPreyCommon.c ../PredPreyCommon.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -o $@ -c $<

clean:
	rm -f *.o bin/*
