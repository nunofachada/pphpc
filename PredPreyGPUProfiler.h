#ifndef PREDPREYPROFILER_H
#define PREDPREYPROFILER_H

#include <glib.h>
#include "PredPreyCommon.h"

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

/** 
 * @brief Create a new OpenCL events profile.
 * 
 * @return A new profile or NULL if operation failed. 
 */
ProfCLProfile* profcl_profile_new();

/** 
 * @brief Free an OpenCL events profile.
 * 
 * @param profile OpenCL events profile to destroy. 
 */
void profcl_profile_free(ProfCLProfile* profile);

/**
 * @brief Add OpenCL event to events profile, more specifically adds 
 * the start and end instants of the given event to the profile.
 * 
 * @param profile OpenCL events profile.
 * @param event_name Event name.
 * @param ev CL event data structure.
 * @return CL_SUCCESS if operation successful, or a flag returned by 
 *          clGetEventProfilingInfo otherwise.
 */ 
cl_uint profcl_profile_add(ProfCLProfile* profile, const char* event_name, cl_event ev);

/** 
 * @brief Create new event instant.
 * 
 * @param eventName Name of event.
 * @param id Id of event.
 * @param instant Even instant in nanoseconds.
 * @param type Type of event instant: PROFCL_EV_START or PROFCL_EV_END
 * @return A new event instant or NULL if operation failed.
 */
ProfCLEvInst* profcl_evinst_new(const char* eventName, guint id, cl_ulong instant, ProfCLEvInstType type);

/**
 * @brief Free an event instant.
 * 
 * @param event_instant Event instant to destroy. 
 */
void profcl_evinst_free(gpointer event_instant);

/**
 * @brief Compares two event instants for sorting within a GList. It is
 * an implementation of GCompareFunc() from GLib.
 * 
 * @param a First event instant to compare.
 * @param b Second event instant to compare.
 * @return Negative value if a < b; zero if a = b; positive value if a > b.
 */
gint profcl_evinst_comp(gconstpointer a, gconstpointer b);

/**
 * @brief Frees an event overlap matrix.
 * 
 * @param An event overlap matrix.
 */
 void profcl_overmat_free(ProfCLOvermat overmat);
	
/**
 * @brief Create new event overlap matrix given an OpenCL events profile.
 * 
 * @param profile An OpenCL events profile.
 * @return A new event overlap matrix or NULL if operation failed.
 */
ProfCLOvermat profcl_overmat_new(ProfCLProfile* profile);

/**
 * @brief Print profiling info.
 * 
 * @param profile An OpenCL events profile.
 * @param dt
  */
void printProfilingInfo(ProfCLProfile* profile, double dt);

#endif


