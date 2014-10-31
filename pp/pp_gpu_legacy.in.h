/**
 * @file
 * Data structures and function headers for the legacy predator-prey
 * OpenCL GPU simulation.
 */

#ifndef _PP_GPU_LEGACY_H_
#define _PP_GPU_LEGACY_H_

#include "pp_common.h"

/** GPU legacy kernel source. */
#define PP_GPU_LEGACY_SRC "@PP_GPU_LEGACY_SRC@"

/** Agent properties. */
typedef struct pp_gs_agent PPGSAgent;

/** Simulation parameters. */
typedef struct pp_gs_sim_params PPGSSimParams;

/* Compute worksizes depending on the device type and number of
 * available compute units. */
void computeWorkSizes(PPParameters params);

/* Print worksizes. */
void printFixedWorkSizes();

/* Get kernel wrappers from program wrapper. */
void getKernelsFromProgram(CCLProgram* prg, GError** err);

#endif
