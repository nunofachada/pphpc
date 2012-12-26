CC = gcc
CFLAGS = -Wall -g -std=gnu99
CLMACROS = -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
LFLAGS = -lOpenCL -lm
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
BASEDIR = ..
OBJDIR = obj
BUILDDIR = bin
UTILSDIR = $(BASEDIR)/utils
UTILOBJS = $(UTILSDIR)/$(OBJDIR)/clerrors.o $(UTILSDIR)/$(OBJDIR)/fileutils.o $(UTILSDIR)/$(OBJDIR)/bitstuff.o $(UTILSDIR)/$(OBJDIR)/clinfo.o

all: mkdirs Matmult
 
Matmult: Matmult.o PredPreyCommon.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

Matmult.o: Matmult.c 
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $(OBJDIR)/$@

PredPreyCommon.o:
	cd $(BASEDIR); make $@
	cp $(BASEDIR)/$(OBJDIR)/$@ $(OBJDIR)

.PHONY: clean mkdirs

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -f *.o bin/*
