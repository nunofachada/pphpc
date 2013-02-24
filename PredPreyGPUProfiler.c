#include "PredPreyGPUProfiler.h"

/** 
 * TODO REMOVE
 * Creates a new event ID.
 *
 * @param id The ID to set.
 * @return A new event ID or NULL if operation failed. 
 */
ProfCLEid profcl_eid_new(guint id) {
	ProfCLEid eid = (profcl_eid) malloc(sizeof(guint));
	return eid;
}

/** 
 * TODO REMOVE
 * Frees an event ID.
 * 
 * @param eid A unique event ID. 
 */
 void profcl_eid_free(ProfCLEid eid) {
	free(eid);
}

/** 
 * Create a new OpenCL events profile.
 * 
 * @return A new profile or NULL if operation failed. 
 */
ProfEvCL* profcl_profile_new() {
	
	/* Allocate memory for new profile data structure. */
	ProfCLProfile* profile = (ProfCLProfile*) malloc(sizeof(ProfCLProfile));

	/* If allocation successful... */
	if (profile != NULL) {
		/* ... create table of unique events, ... */
		profile->unique_events = g_hash_table_new(g_str_hash, g_str_equal);
		/* ... create list of all event instants, ... */
		profile->event_instants = NULL;
		/* ... and set number of event instants to zero. */
		profile->num_event_instants = 0;
	}

	/* Return new profile data structure */
	return profile;
}

/** 
 * Free an OpenCL events profile.
 * 
 * @param profile OpenCL events profile to destroy. 
 */
void profcl_profile_free(ProfCLProfile* profile) {
	/* Destroy table of unique events. */
	g_hash_table_destroy(profile->unique_events);
	/* Destroy list of all event instants. */
	g_list_free_full(profile->event_instants, profcl_evinst_free);
	/* Destroy profile data structure. */
	free(profile);
}

/**
 * Add OpenCL event to events profile.
 * 
 * Add OpenCL event to events profile, more specifically adds the start
 * and end instants of the given event to the profile.
 * 
 * @param profile OpenCL events profile.
 * @param event_name Event name.
 * @param ev CL event data structure.
 * @return CL_SUCCESS if operation successful, or a flag returned by 
 *          clGetEventProfilingInfo otherwise.
 */ 
cl_uint profcl_profile_add(ProfCLProfile* profile, const char* event_name, cl_event ev) {
	
	/* Status flag for OpenCL functions. */
	cl_uint status;
	/* Aux. var. for keeping event instants. */
	cl_ulong instant;
	
	/* Check if event is already registered in the unique events table... */
	if (!g_hash_table_lookup_extended(profile->unique_events, event_name, NULL, NULL) {
		/* ...if not, register it. */
		ProfCLEid unique_event_id = GUINT_TO_POINTER(g_hash_table_size(profile->unique_events));
		g_hash_table_insert(profile->unique_events, event_name, unique_event_id);
	}
	
	/* Update number of event instants, and get an ID for the given event. */
	guint event_id = ++profile->num_event_instants;
	
	/* Add event start instant to list of event instants. */
	status = clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &instant, NULL);
	if (status != CL_SUCCESS) return status;
	ProfCLEvInst* evinst_start = profcl_evinst_new(event_name, event_id, instant, PROFCL_EV_START);
	profile->event_instants = g_list_prepend(profile->event_instants, (gpointer) evinst_start);

	/* Add event end instant to list of event instants. */
	status = clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &instant, NULL);
	if (status != CL_SUCCESS) return status;
	ProfCLEvInst* evinst_end = profcl_evinst_new(event_name, event_id, instant, PROFCL_EV_END);
	profile->event_instants = g_list_prepend(profile->event_instants, (gpointer) evinst_end);
	
	/* Return success flag. */
	return CL_SUCCESS;
	
}

/** 
 * Create new event instant.
 * 
 * @param eventName Name of event.
 * @param id Id of event.
 * @param instant Even instant in nanoseconds.
 * @param type Type of event instant: PROFCL_EV_START or PROFCL_EV_END
 * @return A new event instant or NULL if operation failed.
 */
ProfCLEvInst* profcl_evinst_new(char* eventName, guint id, cl_ulong instant, ProfCLEvInstType type) {
	
	/* Allocate memory for event instant data structure. */
	ProfCLEvInst* event_instant = (ProfCLEvInst*) malloc(sizeof(ProfCLEvInst));
	/* If allocation successful... */
	if (event_instant != NULL) {
		/* ...initialize structure fields. */
		event_instant->eventName = eventName;
		event_instant->id = profcl_eid_new(id);
		event_instant->instant = instant;
		event_instant->type = type;
	}
	/* Return event instant data structure. */
	return event_instant;	
}

/**
 * Free an event instant.
 * 
 * @param event_instant Event instant to destroy. 
 */
void profcl_evinst_free(ProfCLEvInst* event_instant) {
	profcl_eid_free(event_instant->id);
	free(event_instant);
}

/**
 * Compares two event instants for sorting purposes.
 * 
 * Compares two event instants for sorting within a GList. It is an
 * implementation of GCompareFunc() from GLib.
 * 
 * @param a First event instant to compare.
 * @param b Second event instant to compare.
 * @return Negative value if a < b; zero if a = b; positive value if a > b.
 */
gint profcl_evinst_comp(gconstpointer a, gconstpointer b) {
	/* Cast input parameters to event instant data structures. */
	ProfCLEvInst* evInst1 = (ProfCLEvInst*) a;
	ProfCLEvInst* evInst2 = (ProfCLEvInst*) b;
	/* Perform comparison. */
	if (evInst1.instant > evInst2.instant) return  1;
	if (evInst1.instant < evInst2.instant) return -1;
	return 0;
}

/**
 * Frees an event overlap matrix.
 * 
 * @param An event overlap matrix.
 */
 void profcl_overmat_free(ProfCLOvermat overmat) {
	free(overmat);
}
	
/**
 * Create new event overlap matrix given an OpenCL events profile.
 * 
 * @param profile An OpenCL events profile.
 * @return A new event overlap matrix or NULL if operation failed.
 */
ProfCLOvermat profcl_overmat_new(ProfCLProfile* profile) {
	
	/* Determine number of unique events. */
	guint numUniqEvts = g_hash_table_size(profile->unique_events);
	
	/* Initialize overlap matrix. */
	ProfCLOvermat overlapMatrix = (profcl_overmat) malloc(numUniqEvts * numUniqEvts * sizeof(cl_ulong));
	for (guint i = 0; i < numEvents * numEvents; i++)
		overlapMatrix[i] = 0;
		
	/* Initialize helper table to account for all overlapping events. */
	GHashTable* overlaps = g_hash_table_new(g_int_hash, g_int_equal);
	
	/* Setup ocurring events table (key: eventID, value: uniqueEventID) */
	GHashTable* eventsOccurring = g_hash_table_new(g_int_hash, g_int_equal);
		
	/* Sort all event instants. */
	profile->event_instants = g_list_sort(profile->event_instants, profcl_evinst_comp);
	
	/* Iterate through all event instants */
	GList* currEvInstContainer = profile->event_instants;
	while (currEvInstContainer) {
		
		/* Get current event instant. */
		ProfCLEvInst* currEvInst = (ProfCLEvInst*) currEvInstContainer->data;
		
		/* Check if event time is START or END time */
		if (evInst->type == PROFCL_EV_START) { 
			/* Event START instant. */
			
			/* 1 - Check for overlaps with ocurring events */
			GHashTableIter iter;
			gpointer key_eid, value_ueid;
			g_hash_table_iter_init (&iter, eventsOccurring);
			while (g_hash_table_iter_next (&iter, &key_eid, &value_ueid)) {
				guint row = et[i].event < otherEvent ? et[i].event : otherEvent;
				guint col = et[i].event > otherEvent ? et[i].event : otherEvent;
				overlapMatrix[row * numEvents + col] = et[i].instant;
			}

			
			/* 2 - Add event to occurring events. */
			eventsOccurring = g_list_prepend(eventsOccurring, (gpointer) evInst->id);
			
			for (guint otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event < otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event > otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
			// 2 - Add event to ocurring events 
			eventsOcurring[et[i].event] = 1;
			
		} else { /* Event END instant. */
			// 1 - Remove event from ocurring events
			eventsOcurring[et[i].event] = 0;
			// 2 - Check for overlap termination with current events
			for (unsigned int otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event > otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event < otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
		}
		
		/* Get next event instant. */
		currEvInst = currEvInst->next;
	}
	
	// Return current overlap matrix
	return overlapMatrix;
}











// Update simulation profiling data
void updateSimProfile(PROFILE_DATA* profile, EVENTS_CL* events) {
	
	unsigned int numEvents = 4;
	cl_uint status;
	EVENT_TIME et[8];
	cl_ulong grasscount2start, grasscount2end;

	// Grass kernel profiling
	et[0].event = 0; et[0].type = EV_START; 
	et[1].event = 0; et[1].type = EV_END;
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[0].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass start");  exit(EXIT_FAILURE); }
	status = clGetEventProfilingInfo (events->grass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[1].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grass end");  exit(EXIT_FAILURE); }
	profile->grass += et[1].instant - et[0].instant;

	// Count grass 1 kernel profiling
	et[2].event = 1; et[2].type = EV_START; 
	et[3].event = 1; et[3].type = EV_END;
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[2].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->grasscount1, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[3].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount1 end");  exit(EXIT_FAILURE); }
	profile->grasscount1 += et[3].instant - et[2].instant;

	// Grass count 2 kernel profiling
	et[4].event = 2; et[4].type = EV_START; 
	et[5].event = 2; et[5].type = EV_END;
	for (unsigned int i = 0; i < events->grasscount2_num_loops; i++) {

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &grasscount2start, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 start");  exit(EXIT_FAILURE); }
		if (i == 0) et[4].instant = grasscount2start;

		status = clGetEventProfilingInfo (events->grasscount2[i], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &grasscount2end, NULL);
		if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling grasscount2 end");  exit(EXIT_FAILURE); }
		if (i == events->grasscount2_num_loops - 1) et[5].instant = grasscount2end;

		profile->grasscount2 += &grasscount2end - &grasscount2start;
	}
	
	// Profile stats transfer
	et[6].event = 0; et[6].type = EV_START; 
	et[7].event = 0; et[7].type = EV_END;
	status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[6].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->readStats, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[7].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling readStats end");  exit(EXIT_FAILURE); }
	profile->readStats += et[7].instant - et[6].instant;
	
	// Find overlaps
	cl_ulong * currentOverlapMatrix = findOverlaps(et, numEvents);
	
	// Add overlaps to global overlap matrix
	addOverlaps(profile, currentOverlapMatrix, 2, 2 + numEvents - 1);

	// Free current overlap matrix
	free(currentOverlapMatrix);



}

// Update setup data transfer profiling data
void updateSetupProfile(PROFILE_DATA* profile, EVENTS_CL* events) {

	cl_uint status;
	EVENT_TIME et[4];

	// Write grass profiling
	et[0].event = 1; et[0].type = EV_START; 
	et[1].event = 1; et[1].type = EV_END;
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[2].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeGrass, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[3].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeGrass += et[1].instant - et[0].instant;

	// Write Rng profiling
	et[2].event = 1; et[2].type = EV_START; 
	et[3].event = 1; et[3].type = EV_END;
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &et[4].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrasss start"); exit(EXIT_FAILURE);  }
	status = clGetEventProfilingInfo (events->writeRng, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &et[5].instant, NULL);
	if (status != CL_SUCCESS) { PrintErrorGetEventProfilingInfo(status, "profiling writeGrass end");  exit(EXIT_FAILURE); }
	profile->writeRng += et[3].instant - et[2].instant;
	
	// Find overlaps
	cl_ulong * currentOverlapMatrix = findOverlaps(et, 2);
	
	// Add overlaps to global overlap matrix
	addOverlaps(profile, currentOverlapMatrix, 0, 1);

	// Free current overlap matrix
	free(currentOverlapMatrix);


}

// Print profiling info
void printProfilingInfo(PROFILE_DATA* profile, double dt) {
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
	printf("Overlap matrix:\n\n");
	for (unsigned int i = 0; i < NUM_EVENTS; i++) {
		printf("|\t");
		for (unsigned int j = 0; j < NUM_EVENTS; j++) {
			printf("%4.4ld\t", profile->overlapMatrix[i][j]);
		}
		printf("|\n");
	}

	
}

// Function which compares two event times for sorting purposes
int comp (const void * elem1, const void * elem2) {
	EVENT_TIME f = *((EVENT_TIME*) elem1);
	EVENT_TIME s = *((EVENT_TIME*) elem2);
	if (f.instant > s.instant) return  1;
	if (f.instant < s.instant) return -1;
	return 0;
}

cl_ulong * findOverlaps(EVENT_TIME * et, unsigned int numEvents) {
	
	// Setup current overlap matrix
	cl_ulong *currentOverlapMatrix = (cl_ulong *) malloc(numEvents * numEvents * sizeof(cl_ulong));
	for (unsigned int i = 0; i < numEvents * numEvents; i++)
		currentOverlapMatrix[i] = 0;
	
	// Setup ocurring events array
	unsigned char eventsOcurring[numEvents];
	for (unsigned int i = 0; i < numEvents; i++)
		eventsOcurring[i] = 0;
	// Sort et (event times array)
	qsort(et, (size_t) 2 * numEvents, sizeof(EVENT_TIME), comp);
	
	// Iterate through each element of et (event times)
	for (unsigned int i = 0; i < numEvents * 2; i++) {
	
		// Check if event time is START or END time
		if (et[i].type == EV_START) {
			// 1 - Check for new overlap with ocurring events
			for (unsigned int otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event < otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event > otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
			// 2 - Add event to ocurring events 
			eventsOcurring[et[i].event] = 1;
		} else {
			// 1 - Remove event from ocurring events
			eventsOcurring[et[i].event] = 0;
			// 2 - Check for overlap termination with current events
			for (unsigned int otherEvent = 0; otherEvent < numEvents; otherEvent++) {
				if (eventsOcurring[otherEvent]) {
					unsigned int row = et[i].event > otherEvent ? et[i].event : otherEvent;
					unsigned int col = et[i].event < otherEvent ? et[i].event : otherEvent;
					currentOverlapMatrix[row * numEvents + col] = et[i].instant;
				}
			}
		}

	}
	// Return current overlap matrix
	return currentOverlapMatrix;
}

void addOverlaps(PROFILE_DATA * profile, cl_ulong *currentOverlapMatrix, unsigned int startIdx, unsigned int endIdx) {
	unsigned int numEvents = endIdx - startIdx + 1;
	for (unsigned int i = 0; i < numEvents; i++) {
		for (unsigned int j = i + 1; j < numEvents; j++) {
			profile->overlapMatrix[startIdx + i][startIdx + j] += currentOverlapMatrix[j * numEvents + i] - currentOverlapMatrix[i * numEvents + j];
		}
	}
}
