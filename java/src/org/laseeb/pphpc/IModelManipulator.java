/*
 * Copyright (c) 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Instituto Superior Técnico nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *     
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

import java.util.Random;

/**
 * Interface which defines methods for model manipulation.
 *  
 * @author Nuno Fachada
 */
public interface IModelManipulator {
	
	/**
	 * Reset model.
	 */
	public void reset();
	
	/**
	 * Increment iteration.
	 */
	public void incrementIteration();
	
	/**
	 * Notify model of simulation start.
	 */
	public void start();

	/**
	 * Notify model of simulation stop.
	 */
	public void stop();

	/**
	 * Notify model of simulation pause.
	 */
	public void pause();
	
	/**
	 * Notify model of simulation unpause.
	 */
	public void unpause();

	/**
	 * Export statistics to file.
	 * 
	 * @param filename Statistics filename.
	 */
	public void export(String filename);

	/**
	 * Get a model cell from the specified space-independent index.
	 * 
	 * @param idx Space-independent index from where to fetch the cell.
	 * @return A model cell from the specified space-independent index.
	 */
	public ICell getCell(int idx);
	
	/**
	 * Set the neighbors of the model cell located at the specified space-independent 
	 * index. This method can only be called once after all cells have been initialized
	 * with {@link #initCellAt(int, Random)}.
	 * 
	 * @param idx Space-independent index of the cell in which the neighbors will be set.
	 */
	public void setCellNeighbors(int idx);

	/**
	 * Initialize a model cell at the specified space-independent index.
	 * 
	 * @param idx Space-independent index where to place the cell.
	 * @param rng Random number generator with which to initialize the cell.
	 */
	public void initCellAt(int idx, Random rng);

	/**
	 * Update global statistics for a specified iteration. The given 
	 * iteration statistics are added to the global statistics for the 
	 * specified iteration.
	 * 
	 * @param iter Iteration.
	 * @param iterStats Statistics to add.
	 */
	public void updateStats(int iter, IterationStats iterStats);
	
}
