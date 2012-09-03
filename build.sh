if [ $1 = 'profiler' ]
then
	profiler='-D CLPROFILER'
	echo "Compiling with PROFILING ON"
else
	profiler='-U CLPROFILER'
	echo "Compiling with PROFILING OFF"
fi

echo "******** Compiling utilities..."
cd utils
./compile.sh
cd ..
echo "******** Compiling common CPU/GPU code..."
g++ -Wall -g -o PredPreyCommon.o -DATI_OS_LINUX -c PredPreyCommon.cpp -I$AMDAPPSDKROOT/include $profiler
echo "******** Compiling for GPU..."
g++ -Wall -g -o PredPreyGPU.o -DATI_OS_LINUX -c PredPreyGPU.cpp -I$AMDAPPSDKROOT/include $profiler
g++ -Wall -g -o build/PredPreyGPU PredPreyGPU.o PredPreyCommon.o utils/clerrors.o utils/fileutils.o utils/bitstuff.o -lOpenCL -L$AMDAPPSDKROOT/lib/x86_64
echo "******** Compiling for CPU..."
g++ -Wall -g -o PredPreyCPU.o -DATI_OS_LINUX -c PredPreyCPU.cpp -I$AMDAPPSDKROOT/include $profiler
g++ -Wall -g -o build/PredPreyCPU PredPreyCPU.o PredPreyCommon.o utils/clerrors.o utils/fileutils.o utils/bitstuff.o -lOpenCL -L$AMDAPPSDKROOT/lib/x86_64


