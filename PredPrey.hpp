#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "../clutils/clerrors.h"
#include "../clutils/fileutils.h"

typedef struct clzone {
	cl_platform_id platform;
	cl_device_id device;
	cl_uint cu;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
} CLZONE;

CLZONE getClZone(const char* vendor, cl_uint deviceType);
void printAgentArray(cl_uint4* array, unsigned int size);
void printGrassMatrix(cl_uint4* matrix, unsigned int size_x,  unsigned int size_y);

