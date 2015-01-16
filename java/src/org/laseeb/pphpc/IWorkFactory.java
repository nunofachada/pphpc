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

/**
 * Work factories are responsible for creating all the objects related with the
 * execution of a simulation (i.e. simulation work). Different work factories 
 * provide different types of simulation execution, such as single-threaded 
 * execution of various forms of multi-threaded execution.
 * 
 * @author Nuno Fachada
 */
public interface IWorkFactory {

	/**
	 * Creates a simulation controller, in the MVC sense.
	 * 
	 * @param model The simulation model, in the MVC sense. 
	 * @return A new simulation controller.
	 */
	public IController createSimController(IModel model);

	/**
	 * Creates or returns a previously created work provider for a given work size.
	 * 
	 * @param workSize Size of work to be distributed by the work provider.
	 * @param controller The simulation controller.
	 * @return A new or previously created work provider for the given work size.
	 */
	public IWorkProvider getWorkProvider(int workSize, IController controller);
	
	/**
	 * Create and return an appropriate strategy for putting new agents in a cell.
	 * 
	 * @return An appropriate strategy for putting new agents in a cell.
	 */
	public ICellPutAgentStrategy createPutNewAgentStrategy();
	
	/**
	 * Create and return an appropriate strategy for putting existing agents in a cell.
	 * 
	 * @return An appropriate strategy for putting existing agents in a cell.
	 */
	public ICellPutAgentStrategy createPutExistingAgentStrategy();

	/**
	 * Create and return an appropriate global statistics object.
	 * 
	 * @param iters Number of iterations.
	 * @return An appropriate global statistics object.
	 */
	public IGlobalStats createGlobalStats(int iters);
	
	/**
	 * Return number of workers for which this work factory will create work-related
	 * objects.
	 * 
	 * @return Number of workers for which this work factory will create work-related
	 * objects.
	 */
	public int getNumWorkers();
	
	/**
	 * Returns the command name of the work factory. This is used for factory
	 * instantiation.
	 * 
	 * @return The command name of the work factory.
	 */
	public String getCommandName();
	
}
