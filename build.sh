cd utils
./compile.sh
cd ..
g++ -Wall -g -o PredPrey.o -DATI_OS_LINUX -c PredPrey.cpp -I$ATISTREAMSDKROOT/include
g++ -Wall -g -o PredPrey PredPrey.o utils/clerrors.o utils/clutils.o utils/fileutils.o utils/bitstuff.o -lOpenCL -L$ATISTREAMSDKROOT/lib/x86_64


