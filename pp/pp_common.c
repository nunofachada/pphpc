/**
 * @file
 * @brief Implementations of common functions for PredPrey simulation.
 */

#include "pp_common.h"

#define PP_ERROR_MSG_REPEAT "Repeated parameters in parameters file"

/**
 * @brief Load simulation parameters.
 *
 * @param parameters Parameters structure to be populated.
 * @param filename File containing simulation parameters (or `NULL`
 * if default file is to be used).
 * @param err Error structure, to be populated if an error occurs.
 * @return `CL_TRUE` if function terminates successfully, or `CL_FALSE`
 * otherwise.
 * */
int pp_load_params(PPParameters* parameters, char* filename, GError** err) {

	char param[100];
	unsigned int value;
	unsigned int check = 0;
	FILE * fp;
	int status = PP_SUCCESS;
	char* paramsFile = (filename != NULL) ? filename : PP_DEFAULT_PARAMS_FILE;

	fp = fopen(paramsFile, "r");
	ccl_if_err_create_goto(*err, PP_ERROR, fp == NULL,
		PP_UNABLE_TO_OPEN_PARAMS_FILE, error_handler,
		"Unable to open file \"%s\"", paramsFile);

	while (fscanf(fp, "%s = %d", param, &value) != EOF) {
		if (strcmp(param, "INIT_SHEEP") == 0) {
			if ((1 & check) == 0) {
				parameters->init_sheep = value;
				check = check | 1;
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "SHEEP_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 1)) == 0) {
				parameters->sheep_gain_from_food = value;
				check = check | (1 << 1);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "SHEEP_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 2)) == 0) {
				parameters->sheep_reproduce_threshold = value;
				check = check | (1 << 2);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "SHEEP_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 3)) == 0) {
				parameters->sheep_reproduce_prob = value;
				check = check | (1 << 3);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "INIT_WOLVES") == 0) {
			if ((1 & (check >> 4)) == 0) {
				parameters->init_wolves = value;
				check = check | (1 << 4);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "WOLVES_GAIN_FROM_FOOD") == 0) {
			if ((1 & (check >> 5)) == 0) {
				parameters->wolves_gain_from_food = value;
				check = check | (1 << 5);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "WOLVES_REPRODUCE_THRESHOLD") == 0) {
			if ((1 & (check >> 6)) == 0) {
				parameters->wolves_reproduce_threshold = value;
				check = check | (1 << 6);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "WOLVES_REPRODUCE_PROB") == 0) {
			if ((1 & (check >> 7)) == 0) {
				parameters->wolves_reproduce_prob = value;
				check = check | (1 << 7);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "GRASS_RESTART") == 0) {
			if ((1 & (check >> 8)) == 0) {
				parameters->grass_restart = value;
				check = check | (1 << 8);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "GRID_X") == 0) {
			if ((1 & (check >> 9)) == 0) {
				parameters->grid_x = value;
				check = check | (1 << 9);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "GRID_Y") == 0) {
			if ((1 & (check >> 10)) == 0) {
				parameters->grid_y = value;
				check = check | (1 << 10);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else if (strcmp(param, "ITERS") == 0) {
			if ((1 & (check >> 11)) == 0) {
				parameters->iters = value;
				check = check | (1 << 11);
			} else {
				ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
					PP_INVALID_PARAMS_FILE, error_handler,
					PP_ERROR_MSG_REPEAT);
			}
		} else {
			ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
				PP_INVALID_PARAMS_FILE, error_handler,
				"Invalid parameter '%s' in parameters file", param);
		}
	}
	if (check != 0x0fff) {
		ccl_if_err_create_goto(*err, PP_ERROR, TRUE,
			PP_INVALID_PARAMS_FILE, error_handler,
			"Insufficient parameters in parameters file (check=%x)", check);
	}

	/* Set extra utility parameter. */
	parameters->grid_xy = parameters->grid_x * parameters->grid_y;

	/* If we got here, everything is OK. */
	g_assert(err == NULL || *err == NULL);
	status = CL_TRUE;
	goto finish;

error_handler:

	/* If we got here there was an error, verify that it is so. */
	g_assert(err == NULL || *err != NULL);
	status = CL_FALSE;

finish:

	/* Close file. */
	if (fp != NULL) fclose(fp);

	/* Return. */
	return status;
}

/**
 * @brief Callback function which will be called when non-option
 * command line arguments are given. The function will throw an error
 * and fail. It's an implementation of GLib's `(*GOptionArgFunc)`
 * hook function.
 *
 * @param option_name Ignored.
 * @param value Ignored.
 * @param data Ignored.
 * @param err GLib error object for error reporting.
 * @return Always FALSE.
 * */
gboolean pp_args_fail(const gchar *option_name, const gchar *value, gpointer data, GError **err) {
	/* Avoid compiler warning, we're not really using these parameters. */
	(void)option_name;
	(void)value;
	(void)data;
	/* Set error and return FALSE. */
	g_set_error(err, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "This program does not accept non-option arguments.");
	return FALSE;
}

/**
 * @brief Returns the next multiple of a given divisor which is equal or
 * larger than a given value.
 *
 * @param value Minimum value.
 * @param divisor The return value must be a multiple of the divisor.
 * @return The next multiple of a given divisor which is equal or larger
 * than a given value.
 * */
unsigned int pp_next_multiple(unsigned int value, unsigned int divisor) {

	/* The remainder. */
	int rem;

	/* If divisor is 0, then value is the next multiple. */
	if (divisor == 0)
		return value;

	/* Get the remainder. */
	rem = value % divisor;

	/* If remainder is zero, then value is the next multiple... */
	if (rem == 0)
		return value;

	/* ...otherwise just add the divisor and subtract the remainder. */
	return value + divisor - rem;
}

/**
 * @brief Resolves to error category identifying string, in this case an error related to the predator-prey simulation.
 *
 * @return A GQuark structure defined by category identifying string, which identifies the error as a predator-prey simulation generated error.
 */
GQuark pp_error_quark() {
	return g_quark_from_static_string("pp-error-quark");
}
