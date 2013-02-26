#include "PredPreyGPU.h"

void result(char* testName, int testResult) {
	if (testResult) {
		printf("OK: %s\n", testName);
	} else {
		printf("FAIL: %s\n", testName);
	};	
}

int testProfilingOverlaps() {
	
	unsigned int numEvents = 5;
	

	ProfCLProfile* profile = profcl_profile_new();

	// Test with 5 events, 3 overlaps, one of the overlaps has three events
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



	ProfCLOvermat overmat = profcl_overmat_new(profile);

	// Expected matrix
	cl_ulong expectedOvermat[5][5] = 
	{
		{1, 0, 0, 0, 5},
		{0, 0, 3, 1, 0},
		{0, 0, 0, 6, 1},
		{0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0}
	};
	
	int success = 1;
	// Test if currentOverlapMatrix is as expected
	for (unsigned int i = 0; i < numEvents; i++) {
		for (unsigned int j = 0; j < numEvents; j++) {
			if (overmat[i * numEvents + j] != expectedOvermat[i][j]) {
				// Fail!
				success = 0;
				break;
			}
		}
	}

	profcl_overmat_free(overmat);
	profcl_profile_free(profile);
	
	
	// Return test result
	return success;
	
	
}

int main(int argc, char ** argv) {
	result("ProfilingOverlaps", testProfilingOverlaps());
	return 0;

}
