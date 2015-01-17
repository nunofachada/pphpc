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
 * A simulation space with some specific dimensionality and topology.
 * 
 * @author Nuno Fachada
 */
public interface ISpace {

	/**
	 * Set neighbors for cell located at the specified space-independent index. 
	 * 
	 * @param cells Array of all model cells.
	 * @param idx Space-independent index of cell to set the neighbors of.
	 */
	public void setNeighbors(ICell[] cells, int idx);
	
	/**
	 * Return the number of dimensions of this space.
	 * 
	 * @return The number of dimensions of this space.
	 */
	public int getNumDims();
	
	/**
	 * Return the space size in all space dimensions.
	 * 
	 * @return The space size in all space dimensions.
	 */
	public int[] getDims();
	
	/**
	 * Return the space size (number of cells).
	 * 
	 * @return The space size (number of cells).
	 */
	public int getSize();
	
	/**
	 * Return the neighborhood radius.
	 * 
	 * @return The neighborhood radius.
	 */
	public int getNeighborhoodRadius();

}
