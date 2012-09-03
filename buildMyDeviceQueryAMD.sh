gcc -Wall -g -std=c99 -DATI_OS_LINUX MyDeviceQuery.c -I$AMDAPPSDKROOT/include -lOpenCL -L$AMDAPPSDKROOT/lib/x86_6 -o build/MyDeviceQuery
