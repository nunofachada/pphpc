echo "******** Compiling utilities..."
cd utils
./compile.sh
cd ..
echo "******** Compiling common CPU/GPU code..."
g++ -Wall -g -o PredPreyCommon.o -DATI_OS_LINUX -c PredPreyCommon.cpp -I$ATISTREAMSDKROOT/include
echo "******** Compiling for GPU..."
g++ -Wall -g -o PredPreyGPU.o -DATI_OS_LINUX -c PredPreyGPU.cpp -I$ATISTREAMSDKROOT/include
g++ -Wall -g -o PredPreyGPU PredPreyGPU.o PredPreyCommon.o utils/clerrors.o utils/fileutils.o utils/bitstuff.o -lOpenCL -L$ATISTREAMSDKROOT/lib/x86_64
echo "******** Compiling for CPU..."
g++ -Wall -g -o PredPreyCPU.o -DATI_OS_LINUX -c PredPreyCPU.cpp -I$ATISTREAMSDKROOT/include
g++ -Wall -g -o PredPreyCPU PredPreyCPU.o PredPreyCommon.o utils/clerrors.o utils/fileutils.o utils/bitstuff.o -lOpenCL -L$ATISTREAMSDKROOT/lib/x86_64


