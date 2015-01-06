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

public class Square2DTorusSimSpace implements ISimSpace {

	private int x;
//	private int y;
	private int size;
	protected ICell cells[];
	
	public Square2DTorusSimSpace(int x, int y) {
		this.x = x;
//		this.y = y;
		this.size = x * y;
		this.cells = new ICell[size];
	}

	@Override
	public int getSize() {
		return this.size;
	}
	

	@Override
	public ICell getCell(int idx) {
		return this.cells[idx];
	}

	@Override
	public void setCell(int idx, Cell cell) {
		this.cells[idx] = cell;
	}

	@Override
	public void setNeighbors(int idx) {
		
		int up = idx - this.x >= 0 ? idx - this.x : this.size - x + idx;
		int down = idx + this.x < this.size  ? idx + this.x : idx + this.x - this.size;
		int right = idx + 1 < this.size ? idx + 1 : 0;
		int left = idx - 1 >= 0 ? idx - 1 : this.size - 1;
		
		List<ICell> neighborhood = Arrays.asList(
				this.cells[idx], this.cells[up], this.cells[right], this.cells[down], this.cells[left]); 
	
		this.cells[idx].setNeighborhood(Collections.unmodifiableList(neighborhood));
	}
	

}
