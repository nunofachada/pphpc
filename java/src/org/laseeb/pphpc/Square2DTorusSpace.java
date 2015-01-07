package org.laseeb.pphpc;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class Square2DTorusSpace implements ISpace {

	private int x;
	private int size;
	private final int numDims = 2;
	private int[] dims;

	public Square2DTorusSpace(int x, int y) {
		this.x = x;
		this.size = x * y;
		this.dims = new int[] { x, y };
	}
	
	@Override
	public void setNeighbors(ICell[] cells, int idx) {
		
		int up = idx - this.x >= 0 ? idx - this.x : this.size - x + idx;
		int down = idx + this.x < this.size  ? idx + this.x : idx + this.x - this.size;
		int right = idx + 1 < this.size ? idx + 1 : 0;
		int left = idx - 1 >= 0 ? idx - 1 : this.size - 1;
		
		List<ICell> neighborhood = Arrays.asList(
				cells[idx], cells[up], cells[right], cells[down], cells[left]); 
	
		cells[idx].setNeighborhood(Collections.unmodifiableList(neighborhood));
	}

	@Override
	public int getNumDims() {
		return this.numDims;
	}

	@Override
	public int[] getDims() {
		return this.dims;
	}

	@Override
	public int getSize() {
		return this.size;
	}

}
