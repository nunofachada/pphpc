/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Interface for classes which provide management of global simulation statistics.
 * 
 * @author Nuno Fachada
 */
public interface IGlobalStats {
	
	/**
	 * Get a specific statistic for a given iteration.
	 * 
	 * @param st Type of statistic (number of sheep or wolves, or quantity of grass).
	 * @param iter Iteration.
	 * @return The requested statistic.
	 */
	public Number getStats(StatType st, int iter);
	
	/**
	 * Get complete statistics for a given iteration.
	 * 
	 * @param iter Iteration.
	 * @return The complete statistics for a given iteration.
	 */
	public IterationStats getStats(int iter);
	
	/**
	 * Update global statistics for a specified iteration. The given 
	 * iteration statistics are added to the global statistics for the 
	 * specified iteration.
	 * 
	 * @param iter Iteration.
	 * @param stats Statistics to add.
	 */
	public void updateStats(int iter, IterationStats stats);

	/**
	 * Reset statistics (see all statistics in all iteration to zero).
	 */
	public void reset();

}
