# Check if required directories are defined
ifndef OBJDIR
OBJDIR := ../obj
$(warning OBJDIR is not defined, assuming default $(OBJDIR))
endif
ifndef BUILDDIR
BUILDDIR := ../bin
$(warning BUILDDIR is not defined, assuming default $(BUILDDIR))
endif
ifndef CF4OCL_DIR
CF4OCL_DIR := ../cf4ocl
$(warning CF4OCL_DIR is not defined, assuming default $(CF4OCL_DIR))
endif
ifndef CLOPS_DIR
CLOPS_DIR := ../cl-ops
$(warning CLOPS_DIR is not defined, assuming default $(CLOPS_DIR))
endif

# Check if the required OpenCL locations are defined by a higher priority 
# Makefile, if not define them.

# Macros required by a specific OpenCL implementation, e.g. -DATI_OS_LINUX
# for AMDAPPSDK on Linux.
CLMACROS +=

# The location of the OpenCL headers. In Debian/Ubuntu you can install 
# the package opencl-headers, so that CLINCLUDES remains empty. Otherwise
# you should specify the location, e.g. -I$$AMDAPPSDKROOT/include for 
# AMDAPPSDK on Linux.
CLINCLUDES +=

# The location of libOpenCL.so (Linux/Unix) or OpenCL.dll (Windows).
# If you have it in your LD_LIBRARY_PATH (Linux) you can leave CLLIBDIR empty.
# You can leave the location empty if you have installed the ocl-icd-opencl-dev 
# package (Debian/Ubuntu). For AMDAPPSDK on Linux, one would use
# -L$$AMDAPPSDKROOT/lib/x86_64.
CLLIBDIR += 

# Variables Used by Implicit Rules
CC = gcc
CFLAGS = -Wall -Wextra -ggdb3 -std=c99 `pkg-config --cflags glib-2.0` -I$(CF4OCL_DIR)/src/lib -I$(CLOPS_DIR)/src/rng -I$(CLOPS_DIR)/src/sort -I$(CLOPS_DIR)/src/common $(CLMACROS)
LDFLAGS = $(CLLIBDIR)
LDLIBS = -lOpenCL -lm `pkg-config --libs glib-2.0`

# Library objects required for build
UTILOBJS = $(OBJDIR)/clutils.o $(OBJDIR)/clprofiler.o $(OBJDIR)/clerrors.o $(OBJDIR)/clo_common.o $(OBJDIR)/clo_rng.o $(OBJDIR)/clo_sort.o $(OBJDIR)/clo_sort_sbitonic.o

# Phony targets
.PHONY: all clean mkdirs profiling

# Targets and rules
all: mkdirs pp_gpu_legacy pp_cpu pp_gpu

profiling: CFLAGS += -DCLPROFILER
profiling: all

pp_gpu_legacy: pp_gpu_legacy.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LDFLAGS) $(LDLIBS)
	
pp_gpu: pp_gpu.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LDFLAGS) $(LDLIBS)

pp_cpu: pp_cpu.o pp_common.o bitstuff.o
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$@ $(patsubst %,$(OBJDIR)/%,$^) $(UTILOBJS) $(LDFLAGS) $(LDLIBS)

pp_gpu_legacy.o: pp_gpu_legacy.c pp_gpu_legacy.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

pp_gpu.o: pp_gpu.c pp_gpu.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@
	
pp_cpu.o: pp_cpu.c pp_cpu.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

pp_common.o: pp_common.c pp_common.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@
	
bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@	

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)
