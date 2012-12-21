#include "PredPreyGPUProfiler.h"

// Create new profile data
PROFILE_DATA* newProfile() {
	PROFILE_DATA* profile = (PROFILE_DATA*) malloc(sizeof(PROFILE_DATA));
	profile->grass = 0; 
	profile->grasscount1 = 0; 
	profile->grasscount2 = 0;
	profile->writeIter = 0;
	profile->writeGrass = 0;
	profile->writeRng = 0;
	for (unsigned int i = 0; i < NUM_EVENTS; i++) {
		for (unsigned int j = 0; j < NUM_EVENTS; j++) {
			profile->overlapMatrix[i][j] = 0ul;
		}
	}
	return profile;
}

// Free profile data
void freeProfile(PROFILE_DATA* profile) {
	free(profile);
}

// Update simulation profiling data
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events, unsigned char profStats) {
	
	cl_uint status;
	EVENT_TIME et[8];
	cl_ulong grasscount2start, grasscount2end;

	// Grass kernel profiling
	et[0].event = 0; et[0].type = EV_START; 
	et[1].event = 0; et[1].type = EV_END;
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[0].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass start");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[1].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass end");  exit(EXIT_FAILURE); }
	profile->grass += et[1].instant - et[0].instant;

	// Count grass 1 kernel profiling
	et[2].event = 1; et[2].type = EV_START; 
	et[3].event = 1; et[3].type = EV_END;
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[2].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[3].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 end");  exit(EXIT_FAILURE); }
	profile->grasscount1 += et[3].instant - et[2].instant;

	// Grass count 2 kernel profiling
	et[4].event = 2; et[4].type = EV_START; 
	et[5].event = 2; et[5].type = EV_END;
	for (unsigned int i = 0; i < events->grasscount2_num_loops; i++) {

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount2start, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 start");  exit(EXIT_FAILURE); }
		if (i == 0) et[4].instant = grasscount2start;

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount2end, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 end");  exit(EXIT_FAILURE); }
		if (i == events->grasscount2_num_loops - 1) et[5].instant = grasscount2end;

		profile->grasscount2 += &grasscount2end - &grasscount2start;
	}
	
	// Profile stats transfer if required
	unsigned int numEvents;
	if (profStats) {
		numEvents = 4;
		et[6].event = 0; et[6].type = EV_START; 
		et[7].event = 0; et[7].type = EV_END;
		status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[6].instant, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats start"); exit(EXIT_FAILURE);  }
		status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[7].instant, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats end");  exit(EXIT_FAILURE); }
		profile->readStats += et[7].instant - et[6].instant;
	} else {
		numEvents = 3;
	}
	
	// Find overlaps
	cl_ulong * currentOverlapMatrix = findOverlaps(et, numEvents);
	
	// Add overlaps to global overlap matrix
	addOverlaps(profile, currentOverlapMatrix, 3, 3 + numEvents);

	// Free current overlap matrix
	free(currentOverlapMatrix);



}

// Update setup data transfer profiling data
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events) {

	cl_uint status;
	EVENT_TIME et[6];

	// Write iter profiling
	et[0].event = 0; et[0].type = EV_START; 
	et[1].event = 0; et[1].type = EV_END;
	status = clGetEventProfilingInfo (events->writeIter, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[0].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeIter start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeIter, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[1].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeIter end");  exit(EXIT_FAILURE); }
	profile->writeIter += et[1].instant - et[0].instant;

	// Write grass profiling
	et[2].event = 1; et[2].type = EV_START; 
	et[3].event = 1; et[3].type = EV_END;
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[2].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[3].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeGrass += et[3].instant - et[2].instant;

	// Write Rng profiling
	et[4].event = 1; et[4].type = EV_START; 
	et[5].event = 1; et[5].type = EV_END;
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[4].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrasss start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[5].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeRng += et[5].instant - et[4].instant;
	
	// Find overlaps
	cl_ulong * currentOverlapMatrix = findOverlaps(et, 3);
	
	// Add overlaps to global overlap matrix
	addOverlaps(profile, currentOverlapMatrix, 0, 2);

	// Free current overlap matrix
	free(currentOverlapMatrix);


}

// Print profiling info
void printProfilingInfo(PROFILE_DATA* profile, double dt) {
	cl_ulong gpu_profile_total =
		profile->writeIter +
		profile->writeGrass +
		profile->writeRng +
		profile->grass + 
		profile->grasscount1 + 
		profile->grasscount2 +
		profile->readStats;
	double gpu_exclusive = gpu_profile_total * 1e-9;
	double cpu_exclusive = dt - gpu_exclusive;
	printf(", of which %f (%f%%) is CPU and %f (%f%%) is GPU.\n", cpu_exclusive, 100*cpu_exclusive/dt, gpu_exclusive, 100*gpu_exclusive/dt);
	printf("\nGPU timmings:\n\n");
	printf("write iter: %fms (%f%%)\n", profile->writeIter*1e-6, 100*((double) profile->writeIter)/((double) gpu_profile_total));
	printf("write grass: %fms (%f%%)\n", profile->writeGrass*1e-6, 100*((double) profile->writeGrass)/((double) gpu_profile_total));
	printf("write rng: %fms (%f%%)\n", profile->writeRng*1e-6, 100*((double) profile->writeRng)/((double) gpu_profile_total));
	printf("grass: %fms (%f%%)\n", profile->grass*1e-6, 100*((double) profile->grass)/((double) gpu_profile_total));
	printf("grasscount1: %fms (%f%%)\n", profile->grasscount1*1e-6, 100*((double) profile->grasscount1)/((double) gpu_profile_total));
	printf("grasscount2: %fms (%f%%)\n", profile->grasscount2*1e-6, 100*((double) profile->grasscount2)/((double) gpu_profile_total));
	printf("read stats: %fms (%f%%)\n", profile->readStats*1e-6, 100*((double) profile->readStats)/((double) gpu_profile_total));
	printf("\n");
	printf("Overlap matrix:\n\n");
	for (unsigned int i = 0; i < NUM_EVENTS; i++) {
		printf("|\t");
		for (unsigned int j = 0; j < NUM_EVENTS; j++) {
			printf("%4.4ld\t", profile->overlapMatrix[i][j]);
		}
		printf("|\n");
	}

	
}

// Function which compares two event times for sorting purposes
int comp (const void * elem1, const void * elem2) {
	EVENT_TIME f = *((EVENT_TIME*) elem1);
	EVENT_TIME s = *((EVENT_TIME*) elem2);
	if (f.instant > s.instant) return  1;
	if (f.instant < s.instant) return -1;
	return 0;
}

cl_ulong * findOverlaps(EVENT_TIME * et, unsigned int numEvents) {
	
	// Setup current overlap matrix
	cl_ulong *currentOverlapMatrix = (cl_ulong *) malloc(numEvents * numEvents * sizeof(cl_ulong));
	for (unsigned int i = 0; i < numEvents * numEvents; i++)
		currentOverlapMatrix[i] = 0;
	
	// Setup ocurring events array
	unsigned char eventsOcurring[numEvents];
	for (unsigned int i = 0; i < numEvents; i++)
		eventsOcurring[i] = 0;
	// Sort et (event times array)
	qsort(et, (size_t) 2 * numEvents, sizeof(EVENT_TIME), comp);
	
	// Iterate through each element of et (event times)
	for (unsigned int i = 0; i < numEvents * 2; i++) {
	
		// Check if event time is START or END time
		if (et[i].type == EV_START) {
			// 1 - Check for new overlap with ocurring events
			for (unsigned int otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event < otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event > otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
			// 2 - Add event to ocurring events 
			eventsOcurring[et[i].event] = 1;
		} else {
			// 1 - Remove event from ocurring events
			eventsOcurring[et[i].event] = 0;
			// 2 - Check for overlap termination with current events
			for (unsigned int otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event > otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event < otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
		}

	}
	// Return current overlap matrix
	return currentOverlapMatrix;
}

void addOverlaps(PROFILE_DATA * profile, cl_ulong *currentOverlapMatrix, unsigned int startIdx, unsigned int endIdx) {
	unsigned int numEvents = endIdx - startIdx + 1;
	for (unsigned int i = 0; i < numEvents; i++) {
		for (unsigned int j = i + 1; j < numEvents; j++) {
			profile->overlapMatrix[startIdx + i][startIdx + j] += currentOverlapMatrix[j * numEvents + i] - currentOverlapMatrix[i * numEvents + j];
		}
	}
}
