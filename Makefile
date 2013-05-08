# Define required directories
export OBJDIR := ${CURDIR}/obj
export BUILDDIR := ${CURDIR}/bin

UTILSDIR := opencl-utils

SUBDIRS = pp $(UTILSDIR)

# Export utils include directory
export UTILSINCLUDEDIR := ${CURDIR}/$(UTILSDIR)

# Phony targets
.PHONY: all $(SUBDIRS) clean mkdirs getutils

# Targets and rules
all: $(SUBDIRS)

$(SUBDIRS): mkdirs
	$(MAKE) -C $@
     
pp: $(UTILSDIR)

$(UTILSDIR): getutils

getutils:
	test -d $(UTILSDIR) || git clone http://www.laseeb.org/git/$(UTILSDIR)

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)


     
