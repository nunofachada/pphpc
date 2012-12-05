#include "PredPreyGPUProfiler.h"

// Create new profile data
PROFILE_DATA* newProfile() {
	PROFILE_DATA* profile = (PROFILE_DATA*) malloc(sizeof(PROFILE_DATA));
	profile->grass = 0; 
	profile->grasscount1 = 0; 
	profile->grasscount2 = 0;
	return profile;
}

// Free profile data
void freeProfile(PROFILE_DATA* profile) {
	free(profile);
}

// Update profiling data
void updateProfile(PROFILE_DATA* profile, EVENTS_CL* events) {
	
	cl_uint status;
	cl_ulong profile_start, profile_end;

	// Grass kernel profiling
	status = clWaitForEvents(1, &(events->grass));
	if (status != CL_SUCCESS) { PrintErrorWaitForEvents(status, "profiling wait for event grass");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &profile_start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass start");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &profile_end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass end");  exit(EXIT_FAILURE); }
	profile->grass += profile_end - profile_start;

	// Count grass 1 kernel profiling
	status = clWaitForEvents(1, &(events->grasscount1));
	if (status != CL_SUCCESS) { PrintErrorWaitForEvents(status, "profiling wait for event grasscount1");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &profile_start, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &profile_end, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 end");  exit(EXIT_FAILURE); }
	profile->grasscount1 += profile_end - profile_start;

	// Grass count 2 kernel profiling
	for (unsigned int i = 0; i < events->grasscount2_index; i++) {
		status = clWaitForEvents(1, &(events->grasscount2[i]));
		if (status != CL_SUCCESS) { PrintErrorWaitForEvents(status, "profiling wait for event grasscount2");  exit(EXIT_FAILURE); }
		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &profile_start, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 start");  exit(EXIT_FAILURE); }
		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &profile_end, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 end");  exit(EXIT_FAILURE); }
		profile->grasscount2 += profile_end - profile_start;
	}

}

// Print profiling info
void printProfilingInfo(PROFILE_DATA* profile, double dt) {

	cl_ulong gpu_profile_total = profile->grass + profile->grasscount1 + profile->grasscount2;
	double gpu_exclusive = gpu_profile_total * 1e-9;
	double cpu_exclusive = dt - gpu_exclusive;
	printf(", of which %f (%f%%) is CPU and %f (%f%%) is GPU.\n", cpu_exclusive, 100*cpu_exclusive/dt, gpu_exclusive, 100*gpu_exclusive/dt);
	printf("grass: %fms (%f%%)\n", profile->grass*1e-6, 100*((double) profile->grass)/((double) gpu_profile_total));
	printf("grasscount1: %fms (%f%%)\n", profile->grasscount1*1e-6, 100*((double) profile->grasscount1)/((double) gpu_profile_total));
	printf("grasscount2: %fms (%f%%)\n", profile->grasscount2*1e-6, 100*((double) profile->grasscount2)/((double) gpu_profile_total));
	printf("\n");

	
}
