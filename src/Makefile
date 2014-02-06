# Check if required directories are defined
ifndef OBJDIR
OBJDIR := ../obj
$(warning OBJDIR is not defined, assuming default $(OBJDIR))
endif

ifndef BUILDDIR
BUILDDIR := ../bin
$(warning BUILDDIR is not defined, assuming default $(BUILDDIR))
endif

ifndef CLDIR
CLDIR := $(BUILDDIR)/cl
$(warning CLDIR is not defined, assuming default $(CLDIR))
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

# Predator-Prey implementations
PP_IMPL := pp_gpu pp_cpu pp_gpu_legacy

# Binaries associated with the implementations
BINS := $(addprefix $(BUILDDIR)/, $(PP_IMPL))

# Objects associated with the implementations
OBJS := $(addprefix $(OBJDIR)/, $(foreach var,$(PP_IMPL),$(var).o) bitstuff.o pp_common.o)

# Library objects
LIB_OBJS := $(addprefix $(OBJDIR)/,clutils.o clprofiler.o clerrors.o clo_common.o clo_rng.o clo_sort.o clo_sort_sbitonic.o)

# Phony targets
.PHONY : all clean $(PP_IMPL) profiling

# Targets and rules
all: $(PP_IMPL)

profiling: CFLAGS += -DCLPROFILER
profiling: all

$(PP_IMPL) : % : $(BUILDDIR)/%

$(OBJDIR)/%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BINS) : $(LIB_OBJS)
$(BINS) : $(BUILDDIR)/% : $(OBJDIR)/%.o $(OBJDIR)/bitstuff.o $(OBJDIR)/pp_common.o | $(CLDIR)/%.cl $(CLDIR)/pp_common.cl
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(CLDIR)/%.cl : %.cl | $(CLDIR)
	cp $< $@

$(OBJS) : | $(OBJDIR)

$(BINS) : | $(BUILDDIR)

$(OBJDIR) :
	mkdir -p $(OBJDIR)
	
$(BUILDDIR) :
	mkdir -p $(BUILDDIR)
	
$(CLDIR) :
	mkdir -p $(CLDIR)	
	
$(LIB_OBJS):
	$(MAKE) -C $(CF4OCL_DIR)	
	$(MAKE) -C $(CLOPS_DIR)	

clean :
	rm -rf $(OBJDIR) $(BUILDDIR)

