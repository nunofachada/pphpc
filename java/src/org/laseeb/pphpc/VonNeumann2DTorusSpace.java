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

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A two-dimensional toroidal space with Von Neumann neighborhood.
 * 
 * @author Nuno Fachada
 */
public class VonNeumann2DTorusSpace implements ISpace {

	/* Horizontal space size. */
	private int x;
	
	/* Total space size. */
	private int size;
	
	/* Number of space dimensions. */
	private final int numDims = 2;
	
	/* Size of space in each dimension. */
	private int[] dims;

	/**
	 * Create a new two-dimensional toroidal space with Von Neumann neighborhood.
	 * 
	 * @param x Horizontal space size.
	 * @param y Vertical space size.
	 */
	public VonNeumann2DTorusSpace(int x, int y) {
		this.x = x;
		this.size = x * y;
		this.dims = new int[] { x, y };
	}
	
	/**
	 * @see ISpace#setNeighbors(ICell[], int)
	 */
	@Override
	public void setNeighbors(ICell[] cells, int idx) {
		
		/* Determine Von Neumann neighbors. */
		int up = idx - this.x >= 0 ? idx - this.x : this.size - x + idx;
		int down = idx + this.x < this.size  ? idx + this.x : idx + this.x - this.size;
		int right = idx + 1 < this.size ? idx + 1 : 0;
		int left = idx - 1 >= 0 ? idx - 1 : this.size - 1;
		
		/* Set Von Neumann neighborhood. */
		List<ICell> neighborhood = Arrays.asList(
				cells[idx], cells[up], cells[right], cells[down], cells[left]); 
	
		cells[idx].setNeighborhood(Collections.unmodifiableList(neighborhood));
	}

	/**
	 * @see ISpace#getNumDims()
	 */
	@Override
	public int getNumDims() {
		return this.numDims;
	}

	/**
	 * @see ISpace#getDims()
	 */
	@Override
	public int[] getDims() {
		return this.dims;
	}

	/**
	 * @see ISpace#getSize()
	 */
	@Override
	public int getSize() {
		return this.size;
	}

}
