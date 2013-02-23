#ifndef PREDPREYEVENTS_H
#define PREDPREYEVENTS_H
#include <CL/cl.h>

#define NUM_EVENTS 6

typedef struct eventsCL {
	cl_event writeGrass;
	cl_event writeRng;
	cl_event grass; 
	cl_event grasscount1; 
	cl_event* grasscount2;
	cl_event readStats;
	unsigned int grasscount2_num_loops;
	//cl_uint grasscount2_index;
} EVENTS_CL;

EVENTS_CL* newEventsCL();
void freeEventsCL(EVENTS_CL* events);

#endif
