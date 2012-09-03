g++ -g -o PredPrey.o -DATI_OS_LINUX -c PredPrey.cpp -I$ATISTREAMSDKROOT/include
g++ -g -o PredPrey PredPrey.o ../clutils/clerrors.o ../clutils/fileutils.o -lOpenCL -L$ATISTREAMSDKROOT/lib/x86_64


