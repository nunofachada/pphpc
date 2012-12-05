#include "PredPreyGPUEvents.h"

EVENTS_CL* newEventsCL(unsigned int numGrassCount2Events) {
	EVENTS_CL* events = (EVENTS_CL*) malloc(sizeof(EVENTS_CL));
	events->grasscount2 = (cl_event*) malloc(sizeof(cl_event) * numGrassCount2Events);
	events->grasscount2_index = 0;
	return events;
}

void freeEventsCL(EVENTS_CL* events) {
	free(events->grasscount2);
	free(events);
}
