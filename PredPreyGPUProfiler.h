#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include "PredPreyCommon.h"
#include "PredPreyGPUEvents.h"

typedef struct profileData {
	cl_ulong grass; 
	cl_ulong grasscount1; 
	cl_ulong grasscount2;
	cl_ulong readStats;
	cl_ulong writeGrass;
	cl_ulong writeRng;
	cl_ulong overlap;
} PROFILE_DATA;

typedef struct profileTimes {
	cl_ulong start;
	cl_ulong end;
} PROFILE_TIMES;

PROFILE_DATA* newProfile();
void freeProfile(PROFILE_DATA* profile);
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void printProfilingInfo(PROFILE_DATA* profile, double dt);

#endif


