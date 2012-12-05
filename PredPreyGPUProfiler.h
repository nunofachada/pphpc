#include "PredPreyCommon.h"
#include "PredPreyGPUEvents.h"

typedef struct profileData {
	cl_ulong grass; 
	cl_ulong grasscount1; 
	cl_ulong grasscount2;
} PROFILE_DATA;

PROFILE_DATA* newProfile();
void freeProfile(PROFILE_DATA* profile);
void updateProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void printProfilingInfo(PROFILE_DATA* profile, double dt);




