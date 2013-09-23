# Check if required directories are defined
ifndef OBJDIR
OBJDIR := ../obj
$(warning OBJDIR is not defined, assuming default $(OBJDIR))
endif
ifndef BUILDDIR
BUILDDIR := ../bin
$(warning BUILDDIR is not defined, assuming default $(BUILDDIR))
endif
ifndef UTILSINCLUDEDIR
UTILSINCLUDEDIR := ../cf4ocl
$(warning UTILSINCLUDEDIR is not defined, assuming default $(UTILSINCLUDEDIR))
endif

# Define variables
CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -std=c99 `pkg-config --cflags glib-2.0`
CLMACROS += -DATI_OS_LINUX
CLINCLUDES = -I$$AMDAPPSDKROOT/include
UTILSINCLUDES = -I$(UTILSINCLUDEDIR)
LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64 
UTILOBJS = $(OBJDIR)/clutils.o $(OBJDIR)/clprofiler.o
TESTSDIR = tests

# Phony targets
.PHONY: all clean mkdirs profiling

# Targets and rules
all: mkdirs pp_gpu_legacy pp_cpu pp_gpu rng_test

profiling: CLMACROS += -DCLPROFILER
profiling: all

pp_gpu_legacy: pp_gpu_legacy.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)
	
pp_gpu: pp_gpu.o pp_gpu_sort.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

pp_cpu: pp_cpu.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

rng_test: rng_test.o pp_common.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

rng_test.o: rng_test.c
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

pp_gpu_legacy.o: pp_gpu_legacy.c pp_gpu_legacy.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

pp_gpu.o: pp_gpu.c pp_gpu.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@
	
pp_gpu_sort.o: pp_gpu_sort.c pp_gpu_sort.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

pp_cpu.o: pp_cpu.c pp_cpu.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@

pp_common.o: pp_common.c pp_common.h $(UTILSINCLUDEDIR)/gerrorf.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) $(UTILSINCLUDES) -o $(OBJDIR)/$@ -c $<
	
bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@	

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
	cd $(UTILSDIR); make clean
