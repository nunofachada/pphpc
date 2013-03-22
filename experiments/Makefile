CC = gcc
CFLAGS = -Wall -g -std=gnu99 -fopenmp `pkg-config --cflags glib-2.0`
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
OBJDIR = ../obj
BUILDDIR = ../bin
UTILSDIR = ../utils
UTILOBJS = $(UTILSDIR)/$(OBJDIR)/bitstuff.o $(UTILSDIR)/$(OBJDIR)/clutils.o $(UTILSDIR)/$(OBJDIR)/Profiler.o

all: mkdirs BankConflictTest Matmult MatmultT

BankConflictTest: BankConflictTest.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

BankConflictTest.o: BankConflictTest.c 
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@
 
Matmult: Matmult.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

Matmult.o: Matmult.c 
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@

MatmultT: MatmultT.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

MatmultT.o: MatmultT.c 
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@

.PHONY: clean mkdirs

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
