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
 * @brief Common OpenCL kernels and data structures for PredPrey simulation.
 */

/**
 * @brief Performs integer division returning the ceiling instead of
 * the floor if it is not an exact division.
 *
 * @param a Integer numerator.
 * @param b Integer denominator.
 * */
#define PP_DIV_CEIL(a, b) ((a + b - 1) / b)

/**
 * @brief Determines the next multiple of a given divisor which is equal
 * or larger than a given value.
 *
 * Both val and div are assumed to be positive integers.
 *
 * @param val Minimum value.
 * @param div The return value must be a multiple of the divisor.
 * */
#define PP_NEXT_MULTIPLE(val, div) ((val) + (div) - (val) % (div))

/** Sheep identifier. */
#define SHEEP_ID 0x0

/** Wolf identifier. */
#define WOLF_ID 0x1

/**
 * Simulation statistics for one iteration.
 * */
typedef struct pp_statistics_ocl {

	/** Number of sheep. */
	uint sheep;

	/** Number of wolves. */
	uint wolves;

	/** Grass count. */
	uint grass;

	/** Total sheep energy. */
	uint sheep_en;

	/** Total wolves energy. */
	uint wolves_en;

	/** Total grass countdown value. */
	uint grass_en;

	/** Errors during the simulation. */
	uint errors;

} PPStatisticsOcl;



