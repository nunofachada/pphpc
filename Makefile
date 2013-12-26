# Define required directories
export OBJDIR := ${CURDIR}/obj
export BUILDDIR := ${CURDIR}/bin
export CLMACROS

UTILSDIR := cf4ocl/lib

SUBDIRS = pp $(UTILSDIR)

# Export utils include directory
export UTILSINCLUDEDIR := ${CURDIR}/$(UTILSDIR)

# Phony targets
.PHONY: all $(SUBDIRS) clean mkdirs getutils

# Targets and rules
all: $(SUBDIRS)

profiling: CLMACROS=-DCLPROFILER
profiling: all

$(SUBDIRS): mkdirs
	$(MAKE) -C $@
     
pp: $(UTILSDIR)

$(UTILSDIR): getutils

getutils:
	test -d $(UTILSDIR) || git clone https://github.com/FakenMC/cf4ocl.git

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)


     
