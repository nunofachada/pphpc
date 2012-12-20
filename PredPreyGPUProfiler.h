#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include "PredPreyCommon.h"
#include "PredPreyGPUEvents.h"

typedef struct profileData {
	cl_ulong grass; 
	cl_ulong grasscount1; 
	cl_ulong grasscount2;
	cl_ulong readStats;
	cl_ulong writeIter;
	cl_ulong writeGrass;
	cl_ulong writeRng;
	cl_ulong overlapMatrix[NUM_EVENTS][NUM_EVENTS];
} PROFILE_DATA;

/*typedef struct profileTimes {
	cl_ulong start;
	cl_ulong end;
} PROFILE_TIMES;*/

typedef enum {EV_START, EV_END} TIME_TYPE;

typedef struct eventTime {
	cl_ulong instant;
	TIME_TYPE type;
	cl_uint event;
} EVENT_TIME;

PROFILE_DATA* newProfile();
void freeProfile(PROFILE_DATA* profile);
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events, unsigned char profStats);
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void printProfilingInfo(PROFILE_DATA* profile, double dt);
cl_ulong * findOverlaps(EVENT_TIME * et, unsigned int numEvents);
void addOverlaps(PROFILE_DATA profile, cl_ulong *currentOverlapMatrix, unsigned int startIdx, unsigned int endIdx);
#endif


