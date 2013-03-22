#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include <glib.h>
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct profcl_ev_info {
	const char* eventName;
	cl_ulong instantStart;
	cl_ulong instantEnd;
} ProfCLEvInfo;

/** 
 * @typedef 
 * @brief Contains the profiling info of an OpenCL application.
 *
 * @param unique_events Hash table with keys equal to the unique events
 *        name, and values equal to a unique event id.
 * @param event_instants Instants (start and end) of all events occuring
 *        in an OpenCL application.
 * @param num_event_instants Total number of event instants in
 *        event_instants.
 * @param aggregate Aggregate statistics for all events in event_instants.
 * @param overmat Overlap matrix for all events in event_instants.
 * @param totalTime Total time taken by all events.
 * @param totalEffTime Total time taken by all events except intervals
 *        where events overlaped.
 * @param timer Keeps track of time during the complete profiling session.
 */
typedef struct profcl_profile { 
	GHashTable* unique_events;
	GList* event_instants;
	guint num_event_instants;
	GHashTable* aggregate;
	cl_ulong* overmat;
	cl_ulong totalEventsTime;
	cl_ulong totalEventsEffTime;
	GTimer* timer;
} ProfCLProfile;

typedef enum {PROFCL_EV_START, PROFCL_EV_END} ProfCLEvInstType;

typedef enum {PROFCL_EV_SORT_INSTANT, PROFCL_EV_SORT_ID} ProfCLEvSort;

typedef enum {PROFCL_AGGEVDATA_SORT_NAME, PROFCL_AGGEVDATA_SORT_TIME}  ProfCLEvAggDataSort;

typedef struct profcl_evinst { 
	const char* eventName;
	guint id;
	cl_ulong instant;
	ProfCLEvInstType type;
} ProfCLEvInst;

typedef struct profcl_ev_aggregate {
	const char* eventName;
	cl_ulong totalTime;
	double relativeTime;
} ProfCLEvAggregate;

/** @brief Get a structure containing info about a given event. */
ProfCLEvInfo profcl_evinfo_get(const char* event_name, cl_event ev, cl_int* status);

/** @brief Create a new OpenCL events profile. */
ProfCLProfile* profcl_profile_new();

/** @brief Free an OpenCL events profile. */
void profcl_profile_free(ProfCLProfile* profile);

/** @brief Indication that profiling sessions has started. */
void profcl_profile_start(ProfCLProfile* profile);

/** @brief Indication that profiling sessions has ended. */
void profcl_profile_stop(ProfCLProfile* profile);

/** @brief Add OpenCL event to events profile, more specifically adds 
 * the start and end instants of the given event to the profile. */
void profcl_profile_add(ProfCLProfile* profile, ProfCLEvInfo eventInfo);

/** @brief Create new event instant. */
ProfCLEvInst* profcl_evinst_new(const char* eventName, guint id, cl_ulong instant, ProfCLEvInstType type);

/** @brief Free an event instant. */
void profcl_evinst_free(gpointer event_instant);

/** @brief Compares two event instants for sorting purposes. */
gint profcl_evinst_comp(gconstpointer a, gconstpointer b, gpointer userdata);

/** @brief Determine overlap matrix for the given OpenCL events profile. */
void profcl_profile_overmat(ProfCLProfile* profile);

/** @brief Determine aggregate statistics for the given OpenCL events profile. */
void profcl_profile_aggregate(ProfCLProfile* profile);

/** @brief Create a new aggregate statistic for events of a given type. */
ProfCLEvAggregate* profcl_aggregate_new(const char* eventName);

/** @brief Free an aggregate statistic. */
void profcl_aggregate_free(gpointer agg);

/** @brief Print profiling info. */
void profcl_print_info(ProfCLProfile* profile, ProfCLEvAggDataSort evAggSortType);


#endif


