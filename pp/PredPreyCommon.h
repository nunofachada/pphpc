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
 * @defgroup PP_ERROR Error handling.
 *
 * @{
 */
 
/** Error code: successfull operation. */
#define PP_SUCCESS 0 
/** Error code: unknown arguments. */
#define PP_UNKNOWN_ARGS -1 
/** Error code: invalid arguments. */
#define PP_INVALID_ARGS -2
/** Error code: OpenCL error. */
#define PP_OPENCL_ERROR -3
/** Error code: parameters file not found. */
#define PP_UNABLE_TO_OPEN_PARAMS_FILE -4
/** Error code: invalid parameters file. */
#define PP_INVALID_PARAMS_FILE -5


/** If error is detected, create an error object (GError) and go to the specified label. */
#define pp_if_error_create_error_goto(code, err, dest, msg, ...) if (code != PP_SUCCESS) { g_set_error(err, PP_ERROR, code, msg, ##__VA_ARGS__); goto dest; }
/** If error is detected, create an error object (GError) and return from function with given error code. */
#define pp_if_error_create_error_return(code, err, msg, ...) if (code != PP_SUCCESS) { g_set_error(err, PP_ERROR, code, msg, ##__VA_ARGS__); return code; }
/** If error is detected, verify error validity and go to the specified label. */
#define pp_if_error_goto(code, err, dest) if (err != NULL) { g_assert(code != PP_SUCCESS); goto dest; }
/** If error is detected, verify error validity and return from function with given error code. */
#define pp_if_error_return(code, err) if (err != NULL) { g_assert(code != PP_SUCCESS); return code; }

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

/** @brief Resolves to error category identifying string, in this case an error related to the predator-prey simulation. */
GQuark pp_error_quark(void);

#endif
