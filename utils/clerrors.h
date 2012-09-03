#include <CL/cl.h>
#include <stdio.h>

void PrintErrorCreateContext( cl_int error, const char * xtra );
void PrintErrorCreateBuffer ( cl_int error, const char * xtra );
void PrintErrorCreateCommandQueue( cl_int error, const char * xtra );
void PrintErrorGetDeviceInfo( cl_int error, const char * xtra );
void PrintErrorGetDeviceIDs( cl_int error, const char * xtra );
void PrintErrorGetPlatformIDs( cl_int error, const char * xtra );
void PrintErrorSetKernelArg( cl_int error, const char * xtra );
void PrintErrorEnqueueNDRangeKernel( cl_int error, const char * xtra );
void PrintErrorCreateProgramWithSource ( cl_int error, const char * xtra );
void PrintErrorBuildProgram( cl_int error, const char * xtra );
void PrintErrorGetProgramBuildInfo( cl_int error, const char * xtra );
void PrintErrorCreateKernel( cl_int error, const char * xtra );
void PrintErrorEnqueueReadWriteBuffer( cl_int error, const char * xtra );
void PrintErrorWaitForEvents( cl_int error, const char * xtra );
void PrintErrorEnqueueBarrier( cl_int error, const char * xtra );
void PrintErrorEnqueueWaitForEvents( cl_int error, const char * xtra );
void PrintErrorEnqueueMapBuffer( cl_int error, const char * xtra );
void PrintErrorEnqueueUnmapMemObject( cl_int error, const char * xtra );
