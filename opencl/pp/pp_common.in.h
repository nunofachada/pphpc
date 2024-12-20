/*
 * PPHPC-OCL, an OpenCL implementation of the PPHPC agent-based model
 * Copyright (C) 2015 Nuno Fachada
 *
 * This file is part of PPHPC-OCL.
 *
 * PPHPC-OCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PPHPC-OCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PPHPC-OCL. If not, see <http://www.gnu.org/licenses/>.
 * */

/**
 * @file
 * Common data structures and function headers for predator-prey
 * simulation.
 */

#ifndef _PP_COMMON_H_
#define _PP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include <limits.h>
#include <cf4ocl2.h>
#include <cl_ops.h>
#include "_g_err_macros.h"

/** Common kernel source. */
#define PP_COMMON_SRC "@PP_COMMON_SRC@"

/** Default RNG. */
#define PP_RNG_DEFAULT "lcg"

/** Sheep ID. */
#define SHEEP_ID 0

/** Wolf ID. */
#define WOLF_ID 1

/** Default parameters file. */
#define PP_DEFAULT_PARAMS_FILE "config.txt"

/** Default statistics output file. */
#define PP_DEFAULT_STATS_FILE "stats.txt"

/** Default RNG seed. */
#define PP_DEFAULT_SEED 0

/** Resolves to error category identifying string. Required by glib error reporting system. */
#define PP_ERROR pp_error_quark()

/* Perform simulation profiling if PP_PROFILE_OPT symbol is defined. */
#ifdef PP_PROFILE_OPT
	#define PP_QUEUE_PROPERTIES CL_QUEUE_PROFILING_ENABLE
	#define PP_PROFILE TRUE
#else
	#define PP_QUEUE_PROPERTIES 0
	#define PP_PROFILE FALSE
#endif

/**
 * Pointer to compare function to pass to pp_in_array() function.
 *
 * @param[in] elem1 First element to compare.
 * @param[in] elem2 Second element to compare.
 * @return 0 if elements are equal, another value otherwise.
 */
typedef int (*cmpfunc)(void *elem1, void *elem2);

/**
 * Program error codes.
 * */
enum pp_error_codes {
	/** Successfull operation. */
	PP_SUCCESS = 0,
	/** Unknown arguments. */
	PP_UNKNOWN_ARGS = -1,
	/** Arguments are known but invalid. */
	PP_INVALID_ARGS = -2,
	/** Error in external library. */
	PP_LIBRARY_ERROR = -3,
	/** Parameters file not found. */
	PP_UNABLE_TO_OPEN_PARAMS_FILE = -4,
	/** Invalid parameters file. */
	PP_INVALID_PARAMS_FILE = -5,
	/** Unable to save stats. */
	PP_UNABLE_SAVE_STATS = -6,
	/** Program state above limits. */
	PP_OUT_OF_RESOURCES = -8
};

/**
 * Simulation statistics.
 */
typedef struct pp_statistics {
	/** Number of sheep. */
	cl_uint sheep;
	/** Number of wolves. */
	cl_uint wolves;
	/** Quantity of grass. */
	cl_uint grass;
	/** Total sheep energy. */
	cl_uint sheep_en;
	/** Total wolf energy. */
	cl_uint wolves_en;
	/** Total countdown value. */
	cl_uint grass_en;
	/** Simulation errors. */
	cl_uint errors;
} PPStatistics;

/**
 * Simulation parameters.
 */
typedef struct pp_parameters {
	/** Initial number of sheep. */
	unsigned int init_sheep;
	/** Sheep energy gain when eating grass. */
	unsigned int sheep_gain_from_food;
	/** Energy required for sheep to reproduce. */
	unsigned int sheep_reproduce_threshold;
	/** Probability (between 1 and 100) of sheep reproduction. */
	unsigned int sheep_reproduce_prob;
	/** Initial number of wolves. */
	unsigned int init_wolves;
	/** Wolves energy gain when eating sheep. */
	unsigned int wolves_gain_from_food;
	/** Energy required for wolves to reproduce. */
	unsigned int wolves_reproduce_threshold;
	/** Probability (between 1 and 100) of wolves reproduction. */
	unsigned int wolves_reproduce_prob;
	/** Number of iterations that the grass takes to regrow after being
	 * eaten by a sheep. */
	unsigned int grass_restart;
	/** Number of grid columns (horizontal size, width). */
	unsigned int grid_x;
	/** Number of grid rows (vertical size, height). */
	unsigned int grid_y;
	/** Number of grid cells. */
	unsigned int grid_xy;
	/** Number of iterations. */
	unsigned int iters;
} PPParameters;

/**
 * Generic agent parameters.
 */
typedef struct pp_agent_params {
	/** Agent energy gain when eating food. */
	cl_uint gain_from_food;
	/** Energy required for agent to reproduce. */
	cl_uint reproduce_threshold;
	/** Probability (between 1 and 100) of agent reproduction. */
	cl_uint reproduce_prob;
} PPAgentParams;

/* Load predator-prey simulation parameters. */
void pp_load_params(PPParameters * parameters, char * filename, GError ** err);

/* Save simulation statistics. */
void pp_stats_save(char * filename, PPStatistics * statsArray,
	PPParameters params, GError ** err);

/* Export aggregate profiling info to a file. */
void pp_export_prof_agg_info(char * filename, CCLProf * prof);

/* See if there is anything in build log, and if so, show it. */
void pp_build_log(CCLProgram * prg);

/* Callback function which will be called when non-option command line
 * arguments are given. */
gboolean pp_args_fail(const gchar *option_name, const gchar* value,
	gpointer data, GError **err);

/* Returns the next multiple of a given divisor which is equal or
 * larger than a given value. */
cl_int pp_next_multiple(cl_uint value, cl_uint divisor);

/* Resolves to error category identifying string, in this case
 * an error related to the predator-prey simulation. */
GQuark pp_error_quark(void);

#endif
