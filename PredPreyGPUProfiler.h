#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include <glib.h>
#include "PredPreyCommon.h"

typedef struct profcl_ev_info {
	const char* eventName;
	cl_ulong instantStart;
	cl_ulong instantEnd;
} ProfCLEvInfo;

typedef cl_ulong* ProfCLOvermat;

typedef struct profcl_profile { 
	GHashTable* unique_events;
	GList* event_instants;
	guint num_event_instants;
} ProfCLProfile;

typedef enum {PROFCL_EV_START, PROFCL_EV_END} ProfCLEvInstType;

typedef struct profcl_evinst { 
	const char* eventName;
	guint id;
	cl_ulong instant;
	ProfCLEvInstType type;
} ProfCLEvInst;

/** @brief Get a structure containing info about a given event. */
ProfCLEvInfo profcl_evinfo_get(const char* event_name, cl_event ev, cl_int* status);

/** @brief Create a new OpenCL events profile. */
ProfCLProfile* profcl_profile_new();

/** @brief Free an OpenCL events profile. */
void profcl_profile_free(ProfCLProfile* profile);

/** @brief Add OpenCL event to events profile, more specifically adds 
 * the start and end instants of the given event to the profile. */
void profcl_profile_add(ProfCLProfile* profile, ProfCLEvInfo eventInfo);

/** @brief Create new event instant. */
ProfCLEvInst* profcl_evinst_new(const char* eventName, guint id, cl_ulong instant, ProfCLEvInstType type);

/** @brief Free an event instant. */
void profcl_evinst_free(gpointer event_instant);

/** @brief Compares two event instants for sorting purposes. */
gint profcl_evinst_comp(gconstpointer a, gconstpointer b);

/** @brief Frees an event overlap matrix. */
void profcl_overmat_free(ProfCLOvermat overmat);
	
/** @brief Create new event overlap matrix given an OpenCL events profile. */
ProfCLOvermat profcl_overmat_new(ProfCLProfile* profile);

/** @brief Print profiling info. */
void printProfilingInfo(ProfCLProfile* profile, double dt);

#endif


