#CC = gcc
#CFLAGS = -Wall -g -std=gnu99 `pkg-config --cflags glib-2.0`
#CLMACROS = -DATI_OS_LINUX
#CLINCLUDES = -I$$AMDAPPSDKROOT/include
#LFLAGS = -lOpenCL -lm `pkg-config --libs glib-2.0`
#CLLIBDIR = -L$$AMDAPPSDKROOT/lib/x86_64
OBJDIR = obj
BUILDDIR = bin
#UTILSDIR = utils
#UTILOBJS = $(UTILSDIR)/$(OBJDIR)/clutils.o $(UTILSDIR)/$(OBJDIR)/bitstuff.o

SUBDIRS = utils experiments pp
    
all: $(SUBDIRS)

$(SUBDIRS): mkdirs
	$(MAKE) -C $@
     
pp experiments: utils

mkdirs:
	mkdir -p $(BUILDDIR)
	mkdir -p $(OBJDIR)
	
clean:
	rm -rf $(OBJDIR) $(BUILDDIR)

.PHONY: all $(SUBDIRS) clean mkdirs
     
