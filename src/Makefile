# Check if required directories are defined
ifndef OBJDIR
OBJDIR := ../obj
$(warning OBJDIR is not defined, assuming default $(OBJDIR))
endif
ifndef BUILDDIR
BUILDDIR := ../bin
$(warning BUILDDIR is not defined, assuming default $(BUILDDIR))
endif
ifndef CF4OCL_INCDIR
CF4OCL_INCDIR := ../cf4ocl/src/lib
$(warning CF4OCL_INCDIR is not defined, assuming default $(CF4OCL_INCDIR))
endif

# Define variables
CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -std=c99 `pkg-config --cflags glib-2.0`
LIBSINCLUDES = -I$(CF4OCL_INCDIR) -I$(CLOPS_RNG_INCDIR) -I$(CLOPS_SORT_INCDIR) -I$(CLOPS_COMMON_INCDIR)
LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
UTILOBJS = $(OBJDIR)/clutils.o $(OBJDIR)/clprofiler.o $(OBJDIR)/clerrors.o $(OBJDIR)/clo_common.o $(OBJDIR)/clo_rng.o $(OBJDIR)/clo_sort.o $(OBJDIR)/clo_sort_sbitonic.o
TESTSDIR = tests

CLMACROS += 
CLLIBDIR += 
CLINCLUDES += 


# Phony targets
.PHONY: all clean mkdirs profiling

# Targets and rules
all: mkdirs pp_gpu_legacy pp_cpu pp_gpu

profiling: CLMACROS += -DCLPROFILER
profiling: all

pp_gpu_legacy: pp_gpu_legacy.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)
	
pp_gpu: pp_gpu.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

pp_cpu: pp_cpu.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) $(CLMACROS) $(CLLIBDIR) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LFLAGS)

pp_gpu_legacy.o: pp_gpu_legacy.c pp_gpu_legacy.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(LIBSINCLUDES) -o $(OBJDIR)/$@

pp_gpu.o: pp_gpu.c pp_gpu.h
	$(CC) $(CFLAGS) $(CFLAGS_GLIB) $(CLMACROS) -c $< $(CLINCLUDES) $(LIBSINCLUDES) -o $(OBJDIR)/$@
	
pp_cpu.o: pp_cpu.c pp_cpu.h
	$(CC) $(CFLAGS) $(CLMACROS) -c $< $(CLINCLUDES) $(LIBSINCLUDES) -o $(OBJDIR)/$@

pp_common.o: pp_common.c pp_common.h $(CF4OCL_INCDIR)/gerrorf.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) $(LIBSINCLUDES) -o $(OBJDIR)/$@ -c $<
	
bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@	

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR)/* $(BUILDDIR)/*
	cd $(UTILSDIR); make clean
