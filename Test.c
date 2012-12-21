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
	EVENT_TIME et[2 * numEvents];
	PROFILE_DATA* profile = newProfile();

	// Test with 5 events, 3 overlaps, one of the overlaps has three events
	et[0].event = 0; et[0].type = EV_START; et[0].instant = 10;
	et[1].event = 0; et[1].type = EV_END; et[1].instant = 15;
	et[2].event = 1; et[2].type = EV_START; et[2].instant = 16;
	et[3].event = 1; et[3].type = EV_END; et[3].instant = 20;
	et[4].event = 2; et[4].type = EV_START; et[4].instant = 17;
	et[5].event = 2; et[5].type = EV_END; et[5].instant = 30;
	et[6].event = 3; et[6].type = EV_START; et[6].instant = 19;
	et[7].event = 3; et[7].type = EV_END; et[7].instant = 25;
	et[8].event = 4; et[8].type = EV_START; et[8].instant = 29;
	et[9].event = 4; et[9].type = EV_END; et[9].instant = 40;
	
	// Expected matrices
	cl_ulong expectedOverlapMatrix[5][5] = 
	{
		{0, 0, 0, 0, 0},
		{0, 0, 17, 19, 0},
		{0, 20, 0, 19, 29},
		{0, 20, 25, 0, 0},
		{0, 0, 30, 0, 0}
	};
	
	

	
	// Find overlaps
	cl_ulong * currentOverlapMatrix = findOverlaps(et, numEvents);
	
	// Test if currentOverlapMatrix is as expected
	for (unsigned int i = 0; i < numEvents; i++) {
		for (unsigned int j = 0; j < numEvents; j++) {
			if (currentOverlapMatrix[i * numEvents + j] != expectedOverlapMatrix[i][j]) {
				// Fail!
				return 0;
			}
		}
	}

	// Add overlaps to global overlap matrix
	addOverlaps(profile, currentOverlapMatrix, 0, numEvents - 1);


	// Free current overlap matrix
	free(currentOverlapMatrix);
	
	// Check if final overlap matrix is as expected
	if (profile->overlapMatrix[1][2] != 3)
		return 0;
	if (profile->overlapMatrix[1][3] != 1)
		return 0;
	if (profile->overlapMatrix[2][3] != 6)
		return 0;
	if (profile->overlapMatrix[2][4] != 1)
		return 0;
	
	
	// Return test result
	return 1;
	
	
}

int main(int argc, char ** argv) {
	result("ProfilingOverlaps", testProfilingOverlaps());
	return 0;

}
