CC=gcc
CFLAGS=-Wall -g -std=c99
CLMACROS=-DATI_OS_LINUX
CLINCLUDES=-I$AMDAPPSDKROOT/include
CLLIB=-lOpenCL
CLLIBDIR=-L$AMDAPPSDKROOT/lib/x86_64
 
all: MyDeviceQuery clerrors.o fileutils.o bitstuff.o clinfo.o

MyDeviceQuery: MyDeviceQuery.c clinfo.o
	$(CC) $(CFLAGS) $(CLMACROS) $^ $(CLINCLUDES) $(CLLIB) $(CLLIBDIR) -o $@

clerrors.o: clerrors.c clerrors.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $@	

clinfo.o: clinfo.c clinfo.h
	$(CC) $(CFLAGS) $(CLMACROS) $(CLINCLUDES) -c $< -o $@	

fileutils.o: fileutils.c fileutils.h
	$(CC) $(CFLAGS) -c $< -o $@

bitstuff.o: bitstuff.c bitstuff.h
	$(CC) $(CFLAGS) -c $< -o $@
	
clean:
	rm -f *.o MyDeviceQuery

