#ifndef PREDPREYEVENTS_H
#define PREDPREYEVENTS_H
#include <CL/cl.h>

typedef struct eventsCL {
	cl_event mapGrass;
	cl_event unmapGrass;
	cl_event mapRng;
	cl_event unmapRng;
	cl_event grass; 
	cl_event grasscount1; 
	cl_event* grasscount2;
	unsigned int grasscount2_num_loops;
	cl_uint grasscount2_index;
} EVENTS_CL;

EVENTS_CL* newEventsCL();
void freeEventsCL(EVENTS_CL* events);

#endif
