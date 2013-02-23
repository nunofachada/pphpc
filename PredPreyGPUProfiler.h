#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include <glib.h>
#include "PredPreyCommon.h"

typedef struct profcl_profile { 
	GHashTable* unique_events;
	GList* all_events;
} ProfCLProfile;

typedef enum {PROFCL_EV_START, PROFCL_EV_END} ProfCLEvInstType;

typedef struct profcl_evinst { 
	char* eventName;
	cl_ulong instant;
	ProfCLEvInstType type;
} ProfCLEvInst;

ProfileEvCL* newProfile();
void freeProfile(PROFILE_DATA* profile);
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void printProfilingInfo(PROFILE_DATA* profile, double dt);
cl_ulong * findOverlaps(EVENT_TIME * et, unsigned int numEvents);
void addOverlaps(PROFILE_DATA * profile, cl_ulong *currentOverlapMatrix, unsigned int startIdx, unsigned int endIdx);
#endif


