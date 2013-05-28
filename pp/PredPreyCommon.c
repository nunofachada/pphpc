/** 
 * @file
 * @brief Implementations of common functions for PredPrey simulation.
 */

#include "PredPreyCommon.h"

/**
 * @brief Load simulation parameters
 * 
 * @param paramsFile File containing simulation parameters.
 * */
PPParameters pp_load_params(const char* paramsFile) {
	PPParameters parameters;
	FILE * fp = fopen(paramsFile, "r");
	if(fp == NULL) {
		printf("Error: File 'config.txt' not found!\n");
		exit(-1);
	}	
	char param[100];
	unsigned int value;
	unsigned int check = 0;
	while (fscanf(fp, "%s = %d", param, &value) != EOF) {
		if (strcmp(param, "INIT_SHEEP") == 0) {
			if ((1 & check) == 0) {
				parameters.init_sheep = value;
				check = check | 1;
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 1)) == 0) {
				parameters.sheep_gain_from_food = value;
				check = check | (1 << 1);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 2)) == 0) {
				parameters.sheep_reproduce_threshold = value;
				check = check | (1 << 2);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 3)) == 0) {
				parameters.sheep_reproduce_prob = value;
				check = check | (1 << 3);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "INIT_WOLVES") == 0) {
			if ((1 & (check >> 4)) == 0) {
				parameters.init_wolves = value;
				check = check | (1 << 4);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 5)) == 0) {
				parameters.wolves_gain_from_food = value;
				check = check | (1 << 5);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 6)) == 0) {
				parameters.wolves_reproduce_threshold = value;
				check = check | (1 << 6);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 7)) == 0) {
				parameters.wolves_reproduce_prob = value;
				check = check | (1 << 7);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRASS_RESTART") == 0) {
			if ((1 & (check >> 8)) == 0) {
				parameters.grass_restart = value;
				check = check | (1 << 8);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRID_X") == 0) {
			if ((1 & (check >> 9)) == 0) {
				parameters.grid_x = value;
				check = check | (1 << 9);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "GRID_Y") == 0) {
			if ((1 & (check >> 10)) == 0) {
				parameters.grid_y = value;
				check = check | (1 << 10);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else if (strcmp(param, "ITERS") == 0) {
			if ((1 & (check >> 11)) == 0) {
				parameters.iters = value;
				check = check | (1 << 11);
			} else {
				printf("Error: Repeated parameters in config file!\n");
				exit(-1);
			}	
		} else {
			printf("Error: Invalid parameter '%s' in config file!\n", param);
			exit(-1);
		}
	}
	if (check != 0x0fff) {
			printf("Error: Insufficient parameters in config file (check=%d)!\n", check);
			exit(-1);
	}
	return parameters;
} 


