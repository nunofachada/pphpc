#include "PredPreyGPUProfiler.h"

/** 
 * @detail Get a structure containing info about a given event. 
 * 
 * @param event_name Event name.
 * @param ev Event to get information of.
 * @param status Will contain CL_SUCCESS if info about event is
 *         successfully obtain, or an error code otherwise.
 * @return Structure containing info about the given event.
 */
ProfCLEvInfo profcl_evinfo_get(const char* event_name, cl_event ev, cl_int* status) {
	
	/* Create event info structure. */
	ProfCLEvInfo evInfo;
	evInfo.eventName = event_name;
	cl_ulong instant;
	
	/* Get event start instant, add it to event info structure. */
	*status = clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &instant, NULL);
	if (*status != CL_SUCCESS) return evInfo;
	evInfo.instantStart = instant;

	/* Get event end instant, add it to event info structure. */
	*status = clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &instant, NULL);
	if (*status != CL_SUCCESS) return evInfo;
	evInfo.instantEnd = instant;

	return evInfo;
}

/** 
 * @detail Create a new OpenCL events profile.
 * 
 * @return A new profile or NULL if operation failed. 
 */
ProfCLProfile* profcl_profile_new() {
	
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
 * @detail Free an OpenCL events profile.
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
 * @detail Add OpenCL event to events profile, more specifically adds 
 * the start and end instants of the given event to the profile.
 * 
 * @param profile OpenCL events profile.
 * @param event_name Event name.
 * @param ev CL event data structure.
 */ 
void profcl_profile_add(ProfCLProfile* profile, ProfCLEvInfo eventInfo) {
	
	/* Check if event is already registered in the unique events table... */
	if (!g_hash_table_contains(profile->unique_events, eventInfo.eventName)) {
		/* ...if not, register it. */
		guint* unique_event_id = GUINT_TO_POINTER(g_hash_table_size(profile->unique_events));
		g_hash_table_insert(profile->unique_events, (gpointer) eventInfo.eventName, (gpointer) unique_event_id);
	}
	
	/* Update number of event instants, and get an ID for the given event. */
	guint event_id = ++profile->num_event_instants;
	
	/* Add event start instant to list of event instants. */
	ProfCLEvInst* evinst_start = profcl_evinst_new(eventInfo.eventName, event_id, eventInfo.instantStart, PROFCL_EV_START);
	profile->event_instants = g_list_prepend(profile->event_instants, (gpointer) evinst_start);

	/* Add event end instant to list of event instants. */
	ProfCLEvInst* evinst_end = profcl_evinst_new(eventInfo.eventName, event_id, eventInfo.instantEnd, PROFCL_EV_END);
	profile->event_instants = g_list_prepend(profile->event_instants, (gpointer) evinst_end);
	
}

/** 
 * @detail Create new event instant.
 * 
 * @param eventName Name of event.
 * @param id Id of event.
 * @param instant Even instant in nanoseconds.
 * @param type Type of event instant: PROFCL_EV_START or PROFCL_EV_END
 * @return A new event instant or NULL if operation failed.
 */
ProfCLEvInst* profcl_evinst_new(const char* eventName, guint id, cl_ulong instant, ProfCLEvInstType type) {
	
	/* Allocate memory for event instant data structure. */
	ProfCLEvInst* event_instant = (ProfCLEvInst*) malloc(sizeof(ProfCLEvInst));
	/* If allocation successful... */
	if (event_instant != NULL) {
		/* ...initialize structure fields. */
		event_instant->eventName = eventName;
		event_instant->id = id;
		event_instant->instant = instant;
		event_instant->type = type;
	}
	/* Return event instant data structure. */
	return event_instant;	
}


/**
 * @detail Free an event instant.
 * 
 * @param event_instant Event instant to destroy. 
 */
void profcl_evinst_free(gpointer event_instant) {
	free(event_instant);
}

/**
 * @detail Compares two event instants for sorting within a GList. It is
 * an implementation of GCompareFunc() from GLib.
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
	if (evInst1->instant > evInst2->instant) return  1;
	if (evInst1->instant < evInst2->instant) return -1;
	return 0;
}

/**
 * @detail Frees an event overlap matrix.
 * 
 * @param An event overlap matrix.
 */ 
 void profcl_overmat_free(ProfCLOvermat overmat) {
	free(overmat);
}
	
/**
 * @detail Create new event overlap matrix given an OpenCL events profile.
 * 
 * @param profile An OpenCL events profile.
 * @return A new event overlap matrix or NULL if operation failed.
 */ 
ProfCLOvermat profcl_overmat_new(ProfCLProfile* profile) {
	
	/* Determine number of unique events. */
	guint numUniqEvts = g_hash_table_size(profile->unique_events);
	
	/* Initialize overlap matrix. */
	ProfCLOvermat overlapMatrix = (ProfCLOvermat) malloc(numUniqEvts * numUniqEvts * sizeof(cl_ulong));
	for (guint i = 0; i < numUniqEvts * numUniqEvts; i++)
		overlapMatrix[i] = 0;
		
	/* Initialize helper table to account for all overlapping events. */
	GHashTable* overlaps = g_hash_table_new(g_direct_hash, g_direct_equal);
	
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
		if (currEvInst->type == PROFCL_EV_START) { 
			/* Event START instant. */
			
			/* 1 - Check for overlaps with ocurring events */
			GHashTableIter iter;
			gpointer key_eid;
			g_hash_table_iter_init(&iter, eventsOccurring);
			while (g_hash_table_iter_next (&iter, &key_eid, NULL)) {
				/* Inner hash table (is value for overlap hash table). */
				GHashTable* innerTable;
				/* The first hash table key will be the smaller event id. */
				guint eid_key1 = currEvInst->id <= *((guint*) key_eid) ? currEvInst->id : *((guint*) key_eid);
				/* The second hash table key will be the larger event id. */
				guint eid_key2 = currEvInst->id > *((guint*) key_eid) ? currEvInst->id : *((guint*) key_eid);
				/* Check if the first key (smaller id) is already in the hash table... */
				if (!g_hash_table_lookup_extended(overlaps, GUINT_TO_POINTER(eid_key1), NULL, (gpointer) &innerTable)) {
					/* ...if not in table, add it to table, creating a new 
					 * inner table as value. Inner table will be initalized 
					 * with second key (larger id) as key and event start 
					 * instant as value. */
					innerTable = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) g_hash_table_destroy);
					g_hash_table_insert(overlaps, GUINT_TO_POINTER(eid_key1), innerTable);
				}
				/* Add second key (larger id) to inner tabler, setting the 
				 * start instant as the value. */
				g_hash_table_insert(innerTable, GUINT_TO_POINTER(eid_key2), &(currEvInst->instant));
			}

			/* 2 - Add event to occurring events. */
			g_hash_table_insert(
				eventsOccurring, 
				&(currEvInst->id), /* eid */
				g_hash_table_lookup(profile->unique_events, currEvInst->eventName) /* ueid */
			); 
			
		} else {
			/* Event END instant. */
			
			/* 1 - Remove event from ocurring events */
			g_hash_table_remove(eventsOccurring, &(currEvInst->id));
			
			/* 2 - Check for overlap termination with current events */
			GHashTableIter iter;
			gpointer key_eid, ueid_curr_ev, ueid_occu_ev;
			g_hash_table_iter_init(&iter, eventsOccurring);
			while (g_hash_table_iter_next(&iter, &key_eid, &ueid_occu_ev)) {
				/* Inner hash table (is value for overlap hash table). */
				GHashTable* innerTable;
				/* The first hash table key will be the smaller event id. */
				guint eid_key1 = currEvInst->id <= *((guint*) key_eid) ? currEvInst->id : *((guint*) key_eid);
				/* The second hash table key will be the larger event id. */
				guint eid_key2 = currEvInst->id > *((guint*) key_eid) ? currEvInst->id : *((guint*) key_eid);
				/* Get effective overlap in nanoseconds. */
				innerTable = g_hash_table_lookup(overlaps, GUINT_TO_POINTER(eid_key1));
				cl_ulong effOverlap = currEvInst->instant - *((cl_ulong*) g_hash_table_lookup(innerTable, GUINT_TO_POINTER(eid_key2)));
				/* Add overlap to overlap matrix. */
				ueid_curr_ev = g_hash_table_lookup(profile->unique_events, currEvInst->eventName);
				guint ueid_min = GPOINTER_TO_UINT(ueid_curr_ev) <= GPOINTER_TO_UINT(ueid_occu_ev) 
					? GPOINTER_TO_UINT(ueid_curr_ev) 
					: GPOINTER_TO_UINT(ueid_occu_ev);
				guint ueid_max = GPOINTER_TO_UINT(ueid_curr_ev) > GPOINTER_TO_UINT(ueid_occu_ev) 
					? GPOINTER_TO_UINT(ueid_curr_ev) 
					: GPOINTER_TO_UINT(ueid_occu_ev);
				overlapMatrix[ueid_min * numUniqEvts + ueid_max] += effOverlap;
			}	
		}
		
		/* Get next event instant. */
		currEvInstContainer = currEvInstContainer->next;
	}
	
	/* Free overlaps hash table. */
	g_hash_table_destroy(overlaps);
	
	/* Free eventsOccurring hash table. */
	g_hash_table_destroy(eventsOccurring);
	
	/* Return the overlap matrix. */
	return overlapMatrix;
}

/**
 * @detail Print profiling info.
 * 
 * @param profile An OpenCL events profile.
 * @param dt
  */ 
void printProfilingInfo(ProfCLProfile* profile, double dt) {
	/*cl_ulong gpu_profile_total =
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
	}*/

	return;
}
