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
 * Interface which defines methods for model querying.
 *  
 * @author Nuno Fachada
 */
public interface IModelQuerier extends IModelEventObservable {
	
	/**
	 * Get model size (number of cells).
	 * 
	 * @return Model size (number of cells).
	 */
	public int getSize();
	
	/**
	 * Get model parameters.
	 * 
	 * @return Model parameters.
	 */
	public ModelParams getParams();
	
	/**
	 * Get current iteration.
	 * 
	 * @return Current iteration.
	 */
	public int getCurrentIteration();
	
	/**
	 * Get iteration statistics for the specified iteration.
	 * 
	 * @param iteration Iteration to which the requested iteration 
	 * statistics should refer to.
	 * @return Iteration statistics for the specified iteration.
	 */
	public IterationStats getStats(int iteration);
	
	/**
	 * Get iteration statistics for the last iteration for which there
	 * are available statistics.
	 *  
	 * @return Iteration statistics for the last iteration for which there
	 * are available statistics.
	 */
	public IterationStats getLatestStats();

	/**
	 * Register an exception with the model. This will cause the model to
	 * notify all observers of the "exception" model event.
	 * 
	 * @param t Throwable or exception to register.
	 */
	public void registerException(Throwable t);

	/**
	 * Return the last exception registered with {@link #registerException(Throwable)}.
	 * 
	 * @return The last exception registered with {@link #registerException(Throwable)}.
	 */
	public Throwable getLastThrowable();

	/**
	 * Create a random number generator. The type of RNG and the base seed are specified 
	 * at model instantiation time.
	 * 
	 * @param wId Worker ID, required so that each thread gets an independent random number 
	 * generator (not really independent, but good enough for the purpose).
	 * @return A new random number generator.
	 * @throws Exception If for some reason, with wasn't possible to create the RNG.
	 */
	public Random createRNG(int wId) throws Exception;
	
}
