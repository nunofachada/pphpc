#include "Profiler.h"

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
		/* ... and create table of aggregate statistics. */
		profile->aggregate = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, profcl_aggregate_free);
		/* ... and set overlap matrix to NULL. */
		profile->overmat = NULL;
		/* ... and set timer structure to NULL. */
		profile->timer = NULL;
		/* ... and set total times to 0. */
		profile->totalEventsTime = 0;
		profile->totalEventsEffTime = 0;
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
	/* Destroy table of aggregate statistics. */
	g_hash_table_destroy(profile->aggregate);
	/* Free the overlap matrix. */
	if (profile->overmat != NULL)
		free(profile->overmat);
	/* Destroy timer. */
	if (profile->timer != NULL)
		g_timer_destroy(profile->timer);
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
 * an implementation of GCompareDataFunc from GLib.
 * 
 * @param a First event instant to compare.
 * @param b Second event instant to compare.
 * @param userdata Defines what type of sorting to do.
 * @return Negative value if a < b; zero if a = b; positive value if a > b.
 */
gint profcl_evinst_comp(gconstpointer a, gconstpointer b, gpointer userdata) {
	/* Cast input parameters to event instant data structures. */
	ProfCLEvInst* evInst1 = (ProfCLEvInst*) a;
	ProfCLEvInst* evInst2 = (ProfCLEvInst*) b;
	ProfCLEvSort* sortType = (ProfCLEvSort*) userdata;
	/* Perform comparison. */
	if (*sortType == PROFCL_EV_SORT_INSTANT) {
		/* Sort by instant */
		if (evInst1->instant > evInst2->instant) return  1;
		if (evInst1->instant < evInst2->instant) return -1;
	} else if (*sortType == PROFCL_EV_SORT_ID) {
		/* Sort by ID */
		if (evInst1->id > evInst2->id) return 1;
		if (evInst1->id < evInst2->id) return -1;
		if (evInst1->type == PROFCL_EV_END) return 1;
		if (evInst1->type == PROFCL_EV_START) return -1;
	}
	return 0;
}


/**
 * @detail Determine overlap matrix for the given OpenCL events profile.
 *         Must be called after profcl_profile_aggregate.
 * 
 * @param profile An OpenCL events profile.
 */ 
void profcl_profile_overmat(ProfCLProfile* profile) {
	
	/* Total overlap time. */
	cl_ulong totalOverlap = 0;
	
	/* Determine number of unique events. */
	guint numUniqEvts = g_hash_table_size(profile->unique_events);
	
	/* Initialize overlap matrix. */
	cl_ulong* overlapMatrix = (cl_ulong*) malloc(numUniqEvts * numUniqEvts * sizeof(cl_ulong));
	for (guint i = 0; i < numUniqEvts * numUniqEvts; i++)
		overlapMatrix[i] = 0;
		
	/* Initialize helper table to account for all overlapping events. */
	GHashTable* overlaps = g_hash_table_new(g_direct_hash, g_direct_equal);
	
	/* Setup ocurring events table (key: eventID, value: uniqueEventID) */
	GHashTable* eventsOccurring = g_hash_table_new(g_int_hash, g_int_equal);
		
	/* Sort all event instants. */
	ProfCLEvSort sortType = PROFCL_EV_SORT_INSTANT;
	profile->event_instants = g_list_sort_with_data(profile->event_instants, profcl_evinst_comp, (gpointer) &sortType);
	
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
				totalOverlap += effOverlap;
			}	
		}
		
		/* Get next event instant. */
		currEvInstContainer = currEvInstContainer->next;
	}
	
	/* Free overlaps hash table. */
	g_hash_table_destroy(overlaps);
	
	/* Free eventsOccurring hash table. */
	g_hash_table_destroy(eventsOccurring);
	
	/* Add a pointer to overlap matrix to the profile. */
	profile->overmat = overlapMatrix;
	
	/* Determine and save effective events time. */
	profile->totalEventsEffTime = profile->totalEventsTime - totalOverlap;
}

/** 
 * @detail Determine aggregate statistics for the given OpenCL events 
 *         profile. 
 * */
void profcl_profile_aggregate(ProfCLProfile* profile) {
	
	/* Initalize table, and set aggregate values to zero. */
	GHashTableIter iter;
	gpointer eventName;
	g_hash_table_iter_init(&iter, profile->unique_events);
	while (g_hash_table_iter_next(&iter, &eventName, NULL)) {
		ProfCLEvAggregate* evagg = profcl_aggregate_new(eventName);
		evagg->totalTime = 0;
		g_hash_table_insert(profile->aggregate, eventName, (gpointer) evagg);
	}
	
	/* Sort event instants by eid, and then by START, END order. */
	ProfCLEvSort sortType = PROFCL_EV_SORT_ID;
	profile->event_instants = g_list_sort_with_data(profile->event_instants, profcl_evinst_comp, (gpointer) &sortType);

	/* Iterate through all event instants and determine total times. */
	GList* currEvInstContainer = profile->event_instants;
	while (currEvInstContainer) {
		
		ProfCLEvInst* currEvInst;
		
		/* Get START event instant. */
		currEvInst = (ProfCLEvInst*) currEvInstContainer->data;
		cl_ulong startInst = currEvInst->instant;
		
		/* Get END event instant */
		currEvInstContainer = currEvInstContainer->next;
		currEvInst = (ProfCLEvInst*) currEvInstContainer->data;
		cl_ulong endInst = currEvInst->instant;
		
		/* Add new interval to respective aggregate value. */
		ProfCLEvAggregate* currAgg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, currEvInst->eventName);
		currAgg->totalTime += endInst - startInst;
		profile->totalEventsTime += endInst - startInst;
		
		/* Get next START event instant. */
		currEvInstContainer = currEvInstContainer->next;
	}
	
	/* Determine relative times. */
	gpointer valueAgg;
	g_hash_table_iter_init(&iter, profile->aggregate);
	while (g_hash_table_iter_next(&iter, &eventName, &valueAgg)) {
		ProfCLEvAggregate* currAgg = (ProfCLEvAggregate*) valueAgg;
		currAgg->relativeTime = ((double) currAgg->totalTime) / ((double) profile->totalEventsTime);
	}	
	
}

/** 
 * @detail Create a new aggregate statistic for events of a given type.
 * */
ProfCLEvAggregate* profcl_aggregate_new(const char* eventName) {
	ProfCLEvAggregate* agg = (ProfCLEvAggregate*) malloc(sizeof(ProfCLEvAggregate));
	agg->eventName = eventName;
	return agg;
}

/** 
 * @detail Free an aggregate statistic. 
 * */
void profcl_aggregate_free(gpointer agg) {
	free(agg);
}

/**
 * @detail Compares two aggregate event data instances for sorting 
 * within a GList. It is an implementation of GCompareDataFunc from GLib.
 * 
 * @param a First aggregate event data instance to compare.
 * @param b Second aggregate event data instance to compare.
 * @param userdata Defines what type of sorting to do.
 * @return Negative value if a < b; zero if a = b; positive value if a > b.
 */
gint profcl_evagg_comp(gconstpointer a, gconstpointer b, gpointer userdata) {
	/* Cast input parameters to event instant data structures. */
	ProfCLEvAggregate* evAgg1 = (ProfCLEvAggregate*) a;
	ProfCLEvAggregate* evAgg2 = (ProfCLEvAggregate*) b;
	ProfCLEvAggDataSort* sortType = (ProfCLEvAggDataSort*) userdata;
	/* Perform comparison. */
	if (*sortType == PROFCL_AGGEVDATA_SORT_NAME) {
		/* Sort by event name */
		return strcmp(evAgg1->eventName, evAgg2->eventName);
	} else if (*sortType == PROFCL_AGGEVDATA_SORT_TIME) {
		/* Sort by event time period */
		if (evAgg1->totalTime < evAgg2->totalTime) return 1;
		if (evAgg1->totalTime > evAgg2->totalTime) return -1;
	}
	return 0;
}

/** 
* @detail Indication that profiling sessions has started. Starts the
* global profiler timer. 
* */
void profcl_profile_start(ProfCLProfile* profile) {
	profile->timer = g_timer_new();
}

/** 
 * @detail Indication that profiling sessions has ended. Stops the 
 * global profiler timer.
 * */
void profcl_profile_stop(ProfCLProfile* profile) {
	g_timer_stop(profile->timer);
}

/**
 * @detail Print profiling info.
 * 
 * @param profile An OpenCL events profile.
 * @param dt
  */ 
void profcl_print_info(ProfCLProfile* profile, ProfCLEvAggDataSort evAggSortType) {
	
	printf("\n=========================== Timming/Profiling ===========================\n\n");
	
	/* Show total ellapsed time */
	if (profile->timer) {
		printf("- Total ellapsed time:\t%fs\n", g_timer_elapsed(profile->timer, NULL));
	}
	
	/* Show total events time */
	if (profile->totalEventsTime > 0) {
		printf("- Total of all events:\t%fs (100%%)\n", profile->totalEventsTime * 1e-9);
	}
	
	/* Show aggregate event times */
	if (g_hash_table_size(profile->aggregate) > 0) {
		printf("- Aggregate times by event:\n");
		GList* evAggList = g_hash_table_get_values(profile->aggregate);
		evAggList = g_list_sort_with_data(evAggList, profcl_evagg_comp, &evAggSortType);
		GList* evAggContainer = evAggList;
		printf("\t------------------------------------------------------------\n");
		printf("\t| Event name           | Rel. time (%%) | Abs. time (secs.) |\n");
		printf("\t------------------------------------------------------------\n");
		while (evAggContainer) {
			ProfCLEvAggregate* evAgg = (ProfCLEvAggregate*) evAggContainer->data;
			//gchar* eventName = g_strnfill(20, ' ');
			//memcpy(eventName, evAgg->eventName, MIN(strlen(evAgg->eventName), 20) * sizeof(char));
			printf("\t| %-20.20s | %13.4f | %17.4e |\n", evAgg->eventName, evAgg->relativeTime * 100.0, evAgg->totalTime * 1e-9);
			//g_free(eventName);
			evAggContainer = evAggContainer->next;
		}
		printf("\t------------------------------------------------------------\n");
		g_list_free(evAggList);
	}
	
	/* Show overlaps */
	if (profile->overmat) {
		/* Create new temporary hash table which is the reverse in 
		 * terms of key-values of profile->unique_events. */
		GHashTable* tmp = g_hash_table_new(g_direct_hash, g_direct_equal);
		/* Populate temporary hash table. */
		GHashTableIter iter;
		gpointer pEventName, pId;
		g_hash_table_iter_init(&iter, profile->unique_events);
		while (g_hash_table_iter_next(&iter, &pEventName, &pId)) {
			g_hash_table_insert(tmp, pId, pEventName);
		}
		/* Get number of unique events. */
		guint numUniqEvts = g_hash_table_size(profile->unique_events);
		/* Show overlaps. */
		GString* overlapString = g_string_new("");
		for (guint i = 0; i < numUniqEvts; i++) {
			for (guint j = 0; j < numUniqEvts; j++) {
				if (profile->overmat[i * numUniqEvts + j] > 0) {
					g_string_append_printf(
						overlapString,
						"\t| %-20.20s | %-20.20s | %17.4e |\n", 
						(const char*) g_hash_table_lookup(tmp, GUINT_TO_POINTER(i)),
						(const char*) g_hash_table_lookup(tmp, GUINT_TO_POINTER(j)),
						profile->overmat[i * numUniqEvts + j] * 1e-9
					);
				}
			}
		}
		if (strlen(overlapString->str) > 0) {
			/* Show total events effective time (discount overlaps) */
			printf("- Tot. of all events (eff.): %es (saved %es with overlaps)\n", profile->totalEventsEffTime * 1e-9, (profile->totalEventsTime - profile->totalEventsEffTime) * 1e-9);
			/* Title the several overlaps. */
			printf("- Event overlap times:\n");
			printf("\t-------------------------------------------------------------------\n");
			printf("\t| Event 1              | Event2               | Overlap (secs.)   |\n");
			printf("\t-------------------------------------------------------------------\n");
			/* Show overlaps table. */
			printf(overlapString->str);
			printf("\t-------------------------------------------------------------------\n");
		}
		g_string_free(overlapString, TRUE);
				 
		/* Free the hash table. */
		g_hash_table_destroy(tmp);
	}
	
	printf("\n=========================================================================\n");

}
