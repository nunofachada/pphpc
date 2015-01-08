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

public class Model implements IModel {
	
	private int size;
	private SimParams params;
	private ICell cells[];
	private int currentIteration;
	private IGlobalStats globalStats;
	private ISpace space;
	private ICellPutAgentStrategy putNewAgentStrategy;
	private ICellPutAgentStrategy putExistingAgentStrategy;
	private ICellGrassInitStrategy grassInitStrategy;
	
	public Model(SimParams params, IWorkFactory wFactory) {
		this.params = params;
		this.space = new Square2DTorusSpace(params.getGridX(), params.getGridY());
		this.globalStats = wFactory.createGlobalStats(params.getIters());
		this.putNewAgentStrategy = wFactory.createPutNewAgentStrategy();
		this.putExistingAgentStrategy = wFactory.createPutExistingAgentStrategy();
		this.grassInitStrategy = new CellGrassInitCoinRandCounter();
		this.currentIteration = 0;
		this.size = space.getSize();
		this.cells = new ICell[this.size];
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
	public void setCellNeighbors(int idx) {
		this.space.setNeighbors(cells, idx);
	}

	@Override
	public SimParams getParams() {
		return this.params;
	}

	@Override
	public int getCurrentIteration() {
		return this.currentIteration;
	}

	@Override
	public void incrementIteration() {
		this.currentIteration++;
	}

	@Override
	public IGlobalStats getGlobalStats() {
		return this.globalStats;
	}

	@Override
	public void setCellAt(int idx, Random rng) {
		if (this.cells[idx] == null) {
			this.cells[idx] = new Cell(params.getGrassRestart(), 
					this.grassInitStrategy.getInitGrass(params.getGrassRestart(), rng), 
					this.putNewAgentStrategy, this.putExistingAgentStrategy);
		} else {
			throw new IllegalStateException("Cell " + idx + " already set!");
		}
	}

}
