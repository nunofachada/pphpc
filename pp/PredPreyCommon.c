/** 
 * @file
 * @brief Implementations of common functions for PredPrey simulation.
 */

#include "PredPreyCommon.h"

#define ERROR_MSG_REPEAT "Repeated parameters in parameters file"

/**
 * @brief Load simulation parameters.
 * 
 * @param parameters Parameters structure to be populated.
 * @param paramsFile File containing simulation parameters.
 * @param err Error structure, to be populated if an error occurs.
 * @return @link pp_error_codes::PP_SUCCESS @endlink if program terminates successfully,
 * or another value of #pp_error_codes if an error occurs.
 * */
int pp_load_params(PPParameters* parameters, const char* paramsFile, GError** err) {

	char param[100];
	unsigned int value;
	unsigned int check = 0;
	FILE * fp = fopen(paramsFile, "r");
	int status = PP_SUCCESS;

	if(fp == NULL) {
		pp_if_error_create_goto(err, PP_SUCCESS, PP_UNABLE_TO_OPEN_PARAMS_FILE, PP_UNABLE_TO_OPEN_PARAMS_FILE, error_handler, "Unable to open file \"%s\"", paramsFile);
	}	

	while (fscanf(fp, "%s = %d", param, &value) != EOF) {
		if (strcmp(param, "INIT_SHEEP") == 0) {
			if ((1 & check) == 0) {
				parameters->init_sheep = value;
				check = check | 1;
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "SHEEP_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 1)) == 0) {
				parameters->sheep_gain_from_food = value;
				check = check | (1 << 1);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 2)) == 0) {
				parameters->sheep_reproduce_threshold = value;
				check = check | (1 << 2);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "SHEEP_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 3)) == 0) {
				parameters->sheep_reproduce_prob = value;
				check = check | (1 << 3);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "INIT_WOLVES") == 0) {
			if ((1 & (check >> 4)) == 0) {
				parameters->init_wolves = value;
				check = check | (1 << 4);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "WOLVES_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 5)) == 0) {
				parameters->wolves_gain_from_food = value;
				check = check | (1 << 5);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 6)) == 0) {
				parameters->wolves_reproduce_threshold = value;
				check = check | (1 << 6);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "WOLVES_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 7)) == 0) {
				parameters->wolves_reproduce_prob = value;
				check = check | (1 << 7);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "GRASS_RESTART") == 0) {
			if ((1 & (check >> 8)) == 0) {
				parameters->grass_restart = value;
				check = check | (1 << 8);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "GRID_X") == 0) {
			if ((1 & (check >> 9)) == 0) {
				parameters->grid_x = value;
				check = check | (1 << 9);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "GRID_Y") == 0) {
			if ((1 & (check >> 10)) == 0) {
				parameters->grid_y = value;
				check = check | (1 << 10);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else if (strcmp(param, "ITERS") == 0) {
			if ((1 & (check >> 11)) == 0) {
				parameters->iters = value;
				check = check | (1 << 11);
			} else {
				pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, ERROR_MSG_REPEAT);
			}	
		} else {
			pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, "Invalid parameter '%s' in parameters file", param);
		}
	}
	if (check != 0x0fff) {
			pp_if_error_create_goto(err, PP_SUCCESS, PP_INVALID_PARAMS_FILE, PP_INVALID_PARAMS_FILE, error_handler, "Insufficient parameters in parameters file (check=%x)", check);
	}
	
	goto cleanup;
	
error_handler:
	status = (*err)->code;
	
cleanup:
	if (fp != NULL) fclose(fp);
	return status;
} 

/**
 * @brief Show proper error messages.
 * 
 * @param err GLib error structure.
 * @param status Error code.
 * @param zone OpenCL zone, contains build log.
 * */
void pp_error_handle(GError* err, int status, CLUZone zone) {
	g_assert(err != NULL);
	if (zone.build_log) clu_build_log_print(&zone);
	fprintf(stderr, "\n--------------------- Error ---------------------\n");
	fprintf(stderr, "Error code (domain): %d (%s)\nError message: %s\n", err->code, g_quark_to_string(err->domain), err->message);
	fprintf(stderr, "Exit status: %d\n", status);
	fprintf(stderr, "-------------------------------------------------\n");
	g_error_free(err);
}

/** 
 * @brief Resolves to error category identifying string, in this case an error related to the predator-prey simulation.
 * 
 * @return A GQuark structure defined by category identifying string, which identifies the error as a predator-prey simulation generated error.
 */
GQuark pp_error_quark() {
	return g_quark_from_static_string("pp-error-quark");
}
