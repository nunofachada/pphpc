#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include <glib.h>
#include "PredPreyCommon.h"

typedef guint* ProfCLEid;

typedef cl_ulong* ProfCLOvermat;

typedef struct profcl_profile { 
	GHashTable* unique_events;
	GList* event_instants;
	guint num_event_instants;
} ProfCLProfile;

typedef enum {PROFCL_EV_START, PROFCL_EV_END} ProfCLEvInstType;

typedef struct profcl_evinst { 
	char* eventName;
	ProfCLEid id;
	cl_ulong instant;
	ProfCLEvInstType type;
} ProfCLEvInst;


typedef struct profcl_evpair {
	ProfCLEid ev1;
	ProfCLEid ev2;
} ProfCLEvPair;

ProfileEvCL* newProfile();
void freeProfile(PROFILE_DATA* profile);
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events);
void printProfilingInfo(PROFILE_DATA* profile, double dt);
cl_ulong * findOverlaps(EVENT_TIME * et, unsigned int numEvents);
void addOverlaps(PROFILE_DATA * profile, cl_ulong *currentOverlapMatrix, unsigned int startIdx, unsigned int endIdx);
#endif


