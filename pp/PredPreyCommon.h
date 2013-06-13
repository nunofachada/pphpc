/** 
 * @file
 * @brief Common data structures and function headers for PredPrey simulation.
 */

#ifndef PREDPREYCOMMON_H
#define PREDPREYCOMMON_H

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define __NO_STD_STRING

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include <argp.h>
#include <limits.h>
#include "clutils.h"
#include "bitstuff.h"
#include "clprofiler.h"

/** Default parameters file. */
#define DEFAULT_PARAMS_FILE "config.txt"

/** Default statistics output file. */
#define DEFAULT_STATS_FILE "stats.txt"

/** Resolves to error category identifying string. Required by glib error reporting system. */
#define PP_ERROR pp_error_quark()

/**
 * @defgroup PP_ERROR Error handling
 *
 * @{
 */

/**
 * @brief Program error codes.
 * */ 
enum pp_error_codes {
	PP_SUCCESS = 0,						/**< Successfull operation. */
	PP_UNKNOWN_ARGS = -1,				/**< Unknown arguments. */
	PP_INVALID_ARGS = -2,				/**< Invalid arguments. */
	PP_LIBRARY_ERROR = -3,				/**< OpenCL error. */
	PP_UNABLE_TO_OPEN_PARAMS_FILE = -4,	/**< Parameters file not found. */
	PP_INVALID_PARAMS_FILE = -5			/**< Invalid parameters file. */
};


/** 
 * @def pp_if_error_create_handle(err, no_error_code, error_code, pp_error_code, msg, ...)
 * @brief If error is detected, create an error object (GError) and go to the error_handler label. 
 * 
 * @param err GError object.
 * @param no_error_code Successfull operation code.
 * @param error_code Possible error code.
 * @param pp_error_code Error code returned by program if <tt>error_code != no_error_code</tt>.
 * @param msg Error message in case of error.
 * @param ... Extra parameters for error message.
 * */
#define pp_if_error_create_handle(err, no_error_code, error_code, pp_error_code, msg, ...)	\
	if (error_code != no_error_code) {														\
		g_set_error(&err, PP_ERROR, error_code, msg, ##__VA_ARGS__);						\
		error_code = pp_error_code;															\
		goto error_handler;																	\
	}
	
/** 
 * @def pp_if_error_create_return(err, no_error_code, error_code, pp_error_code, msg, ...)
 * @brief If error is detected, create an error object (GError) and return from function with given error code.
 * 
 * @param err GError object.
 * @param no_error_code Successfull operation code.
 * @param error_code Possible error code.
 * @param pp_error_code Error code returned by program if <tt>error_code != no_error_code</tt>.
 * @param msg Error message in case of error.
 * @param ... Extra parameters for error message.
 * */
#define pp_if_error_create_return(err, no_error_code, error_code, pp_error_code, msg, ...)	\
	if (error_code != no_error_code) {														\
		g_set_error(err, PP_ERROR, error_code, msg, ##__VA_ARGS__);							\
		return pp_error_code;																\
	}
	
/** 
 * @def pp_if_error_handle(no_error_code, error_code)
 * @brief If error is detected (<tt>error_code != no_error_code</tt>) go to the specified label.
 * 
 * @param no_error_code Successfull operation code.
 * @param error_code Possible error code.
 * */
 #define pp_if_error_handle(no_error_code, error_code)	\
	if (error_code != no_error_code) goto error_handler;
	
/** 
 * @def pp_if_error_return(no_error_code, error_code, pp_error_code)
 * @brief If error is detected (<tt>error_code != no_error_code</tt>) return from function with return error code.
 * 
 * @param no_error_code Successfull operation code.
 * @param error_code Possible error code.
 * @param pp_error_code Error code to be returned by function.
 * */
#define pp_if_error_return(no_error_code, error_code, pp_error_code)	\
	if (error_code != no_error_code) return pp_error_code;

/**
 * @def pp_error_create_return(err, error_code, msg, ...) 
 * @brief Create error and return from function.
 * 
 * @param err GError object.
 * @param error_code Possible error code.
 * @param msg Error message in case of error.
 * @param ... Extra parameters for error message.
 * */
#define pp_error_create_return(err, error_code, msg, ...)		\
	g_set_error(err, PP_ERROR, error_code, msg, ##__VA_ARGS__);	\
	return error_code;
	
/**
 * @def pp_error_create_handle(err, error_code, msg, ...)
 * @brief Create error and goto error_handler label.
 * 
 * @param err GError object.
 * @param error_code Possible error code.
 * @param msg Error message in case of error.
 * @param ... Extra parameters for error message.
 * */
 #define pp_error_create_handle(err, error_code, msg, ...)		\
	g_set_error(err, PP_ERROR, error_code, msg, ##__VA_ARGS__);	\
	goto error_handler;
	

/** @} */

/** 
 * @brief Simulation statistics.
 */
typedef struct pp_statistics {
	cl_uint sheep;   /**< Number of sheep. */
	cl_uint wolves;  /**< Number of wolves. */
	cl_uint grass;   /**< Quantity of grass. */
} PPStatistics;

/** 
 * @brief Simulation parameters.
 */
typedef struct pp_parameters {
	unsigned int init_sheep;                 /**< Initial number of sheep. */
	unsigned int sheep_gain_from_food;       /**< Sheep energy gain when eating grass. */
	unsigned int sheep_reproduce_threshold;  /**< Energy required for sheep to reproduce. */
	unsigned int sheep_reproduce_prob;       /**< Probability (between 1 and 100) of sheep reproduction. */
	unsigned int init_wolves;                /**< Initial number of wolves. */
	unsigned int wolves_gain_from_food;      /**< Wolves energy gain when eating sheep. */
	unsigned int wolves_reproduce_threshold; /**< Energy required for wolves to reproduce. */
	unsigned int wolves_reproduce_prob;      /**< Probability (between 1 and 100) of wolves reproduction. */
	unsigned int grass_restart;              /**< Number of iterations that the grass takes to regrow after being eaten by a sheep. */
	unsigned int grid_x;  /**< Number of grid columns (horizontal size, width). */
	unsigned int grid_y;  /**< Number of grid rows (vertical size, height). */
	unsigned int iters;   /**< Number of iterations. */
} PPParameters;

/** 
 * @brief Generic agent parameters.
 */
typedef struct pp_agent_params {
	cl_uint gain_from_food;      /**< Agent energy gain when eating food. */
	cl_uint reproduce_threshold; /**< Energy required for agent to reproduce. */
	cl_uint reproduce_prob;      /**< Probability (between 1 and 100) of agent reproduction. */
} PPAgentParams;

/** @brief Load predator-prey simulation parameters. */
int pp_load_params(PPParameters* parameters, const char * paramsFile, GError** err);

/** @brief Show proper error messages. */
void pp_error_handle(GError* err, int status, CLUZone zone);

/** @brief Resolves to error category identifying string, in this case an error related to the predator-prey simulation. */
GQuark pp_error_quark(void);

#endif
