#include "PredPreyGPUProfiler.h"

// Create new profile data
PROFILE_DATA* newProfile() {
	PROFILE_DATA* profile = (PROFILE_DATA*) malloc(sizeof(PROFILE_DATA));
	profile->grass = 0; 
	profile->grasscount1 = 0; 
	profile->grasscount2 = 0;
	profile->writeGrass = 0;
	profile->writeRng = 0;
	profile->overlap = 0;
	return profile;
}

// Free profile data
void freeProfile(PROFILE_DATA* profile) {
	free(profile);
}

// Update simulation profiling data
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events) {
	
	cl_uint status;
	PROFILE_TIMES times[4];
	cl_ulong grasscount2start, grasscount2end;

	// Grass kernel profiling
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &times[0].start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass start");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &times[0].end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass end");  exit(EXIT_FAILURE); }
	profile->grass += &times[0].end - &times[0].start;

	// Count grass 1 kernel profiling
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &times[1].start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &times[1].end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 end");  exit(EXIT_FAILURE); }
	profile->grasscount1 += &times[1].end - &times[1].start;

	// Grass count 2 kernel profiling
	for (unsigned int i = 0; i < events->grasscount2_num_loops; i++) {

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount2start, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 start");  exit(EXIT_FAILURE); }
		if (i == 0) times[2].start = grasscount2start;

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount2end, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 end");  exit(EXIT_FAILURE); }
		if (i == events->grasscount2_num_loops - 1) times[2].end = grasscount2end;

		profile->grasscount2 += &grasscount2end - &grasscount2start;
	}
	
	// Read stats profiling
	status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &times[3].start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &times[3].end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats end");  exit(EXIT_FAILURE); }
	profile->readStats += &times[3].end - &times[3].start;

	// Check for overlaps
	/*for (unsigned int i = 3; i > 0; i--) {
		if (times[i].start < times[].end)
			profile->overlap = times[0].end - times[1].start;
		
	}*/
}

// Update setup data transfer profiling data
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events) {

	cl_uint status;
	PROFILE_TIMES times[2];

	// Write grass profiling
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &times[0].start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrasss start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &times[0].end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeGrass += times[0].end - times[0].start;

	// Write Rng profiling
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &times[1].start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrasss start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &times[1].end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeRng += times[1].end - times[1].start;
	
	// Check for overlap
	if (times[1].start < times[0].end)
		profile->overlap = times[0].end - times[1].start;


}

// Print profiling info
void printProfilingInfo(PROFILE_DATA* profile, double dt) {
// TODO IMPROVE THIS, NAMELY DETERMINE OVERLAPS (IF ANY) DUE TO UNORDERED QUEUE
	cl_ulong gpu_profile_total = 
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
	printf("write grass: %fms (%f%%)\n", profile->writeGrass*1e-6, 100*((double) profile->writeGrass)/((double) gpu_profile_total));
	printf("write rng: %fms (%f%%)\n", profile->writeRng*1e-6, 100*((double) profile->writeRng)/((double) gpu_profile_total));
	printf("grass: %fms (%f%%)\n", profile->grass*1e-6, 100*((double) profile->grass)/((double) gpu_profile_total));
	printf("grasscount1: %fms (%f%%)\n", profile->grasscount1*1e-6, 100*((double) profile->grasscount1)/((double) gpu_profile_total));
	printf("grasscount2: %fms (%f%%)\n", profile->grasscount2*1e-6, 100*((double) profile->grasscount2)/((double) gpu_profile_total));
	printf("read stats: %fms (%f%%)\n", profile->readStats*1e-6, 100*((double) profile->readStats)/((double) gpu_profile_total));
	printf("\n");
	printf("Overlap: %fms\n\n", profile->overlap*1e-6);

	
}
