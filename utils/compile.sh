g++ -o clerrors.o -DATI_OS_LINUX -c clerrors.c -Wall -I$ATISTREAMSDKROOT/include
g++ -o clutils.o -DATI_OS_LINUX -c clutils.c -Wall -I$ATISTREAMSDKROOT/include
g++ -o fileutils.o -c fileutils.c -Wall
g++ -o bitstuff.o -c bitstuff.c -Wall
