# Define required directories
export OBJDIR := ${CURDIR}/obj
export BUILDDIR := ${CURDIR}/bin

export CLMACROS = 
export CLLIBDIR = -L$(AMDAPPSDKROOT)/lib/x86_64 
export CLINCLUDES = -I$(AMDAPPSDKROOT)/include

CF4OCL_DIR := cf4ocl
CLOPS_DIR := cl-ops

SUBDIRS = src $(CLOPS_DIR) $(CF4OCL_DIR)

# Export utils include directory
export CF4OCL_INCDIR := ${CURDIR}/$(CF4OCL_DIR)/src/lib
export CLOPS_RNG_INCDIR := ${CURDIR}/$(CLOPS_DIR)/src/rng
export CLOPS_SORT_INCDIR := ${CURDIR}/$(CLOPS_DIR)/src/sort
export CLOPS_COMMON_INCDIR := ${CURDIR}/$(CLOPS_DIR)/src/common

# Phony targets
.PHONY: all $(SUBDIRS) clean mkdirs getutils

# Targets and rules
all: $(SUBDIRS)

profiling : CLMACROS=-DCLPROFILER
profiling : all

$(SUBDIRS) : mkdirs
	$(MAKE) -C $@
     
src: $(CF4OCL_DIR) $(CLOPS_DIR)

$(CF4OCL_DIR) : get-cf4ocl

$(CLOPS_DIR) : get-clops

get-cf4ocl:
	test -d $(CF4OCL_DIR) || git clone https://github.com/FakenMC/cf4ocl.git

get-clops:
	test -d $(CLOPS_DIR) || git clone https://github.com/FakenMC/cl-ops.git

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)

clean-all: clean
	rm -rf $(CF4OCL_DIR) $(CLOPS_DIR)


     
