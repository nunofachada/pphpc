#include <glib.h>
#include "../PredPreyGPU.h"


static void profilerTest() {
	
	guint numEvents = 5;

	ProfCLProfile* profile = profcl_profile_new();

	// Test with 5 unique events
	ProfCLEvInfo ev1;
	ev1.eventName = "Event 1";
	ev1.instantStart = 10;
	ev1.instantEnd = 15;
	profcl_profile_add(profile, ev1);

	ProfCLEvInfo ev2;
	ev2.eventName = "Event 2";
	ev2.instantStart = 16;
	ev2.instantEnd = 20;
	profcl_profile_add(profile, ev2);

	ProfCLEvInfo ev3;
	ev3.eventName = "Event 3";
	ev3.instantStart = 17;
	ev3.instantEnd = 30;
	profcl_profile_add(profile, ev3);

	ProfCLEvInfo ev4;
	ev4.eventName = "Event 4";
	ev4.instantStart = 19;
	ev4.instantEnd = 25;
	profcl_profile_add(profile, ev4);

	ProfCLEvInfo ev5;
	ev5.eventName = "Event 5";
	ev5.instantStart = 29;
	ev5.instantEnd = 40;
	profcl_profile_add(profile, ev5);

	ProfCLEvInfo ev6;
	ev6.eventName = "Event 1";
	ev6.instantStart = 35;
	ev6.instantEnd = 45;
	profcl_profile_add(profile, ev6);

	ProfCLEvInfo ev7;
	ev7.eventName = "Event 1";
	ev7.instantStart = 68;
	ev7.instantEnd = 69;
	profcl_profile_add(profile, ev7);

	ProfCLEvInfo ev8;
	ev8.eventName = "Event 1";
	ev8.instantStart = 50;
	ev8.instantEnd = 70;
	profcl_profile_add(profile, ev8);

	profcl_profile_aggregate(profile);
	profcl_profile_overmat(profile);
	
	/* ************************* */
	/* Test aggregate statistics */
	/* ************************* */
	
	ProfCLEvAggregate* agg;
	
	agg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, "Event 1");
	g_assert_cmpuint(agg->totalTime, ==, 36);
	g_assert_cmpfloat(agg->relativeTime - 0.51728, <, 0.0001);
	
	agg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, "Event 2");
	g_assert_cmpuint(agg->totalTime, ==, 4);
	g_assert_cmpfloat(agg->relativeTime - 0.05714, <, 0.0001);

	agg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, "Event 3");
	g_assert_cmpuint(agg->totalTime, ==, 13);
	g_assert_cmpfloat(agg->relativeTime - 0.18571, <, 0.0001);

	agg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, "Event 4");
	g_assert_cmpuint(agg->totalTime, ==, 6);
	g_assert_cmpfloat(agg->relativeTime - 0.08571, <, 0.0001);

	agg = (ProfCLEvAggregate*) g_hash_table_lookup(profile->aggregate, "Event 5");
	g_assert_cmpuint(agg->totalTime, ==, 11);
	g_assert_cmpfloat(agg->relativeTime - 0.15714, <, 0.0001);

	/* ********************* */
	/* Test overlap matrix */
	/* ********************* */
	
	/* Expected overlap matrix */
	cl_ulong expectedOvermat[5][5] = 
	{
		{1, 0, 0, 0, 5},
		{0, 0, 3, 1, 0},
		{0, 0, 0, 6, 1},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}
	};
	
	/* Test if currentOverlapMatrix is as expected */
	for (guint i = 0; i < numEvents; i++) {
		for (guint j = 0; j < numEvents; j++) {
			g_assert_cmpuint(profile->overmat[i * numEvents + j], ==, expectedOvermat[i][j]);
		}
	}
	
	//gigprofcl_print_info(profile);

	/* Free profile. */
	profcl_profile_free(profile);
	
}


int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/profiler/profiler", profilerTest);
	return g_test_run();
}

