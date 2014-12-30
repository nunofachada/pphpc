package org.laseeb.pphpc;

public class PPStats {

	private int sheep;
	private int wolves;
	private int grass;

	public PPStats() {}
	
	public void reset() {
		this.sheep = 0;
		this.wolves = 0;
		this.grass = 0;
	}
	
	public int getSheep() {
		return sheep;
	}

	public void incSheep() {
		this.sheep++;
	}

	public int getWolves() {
		return wolves;
	}

	public void incWolves() {
		this.wolves++;
	}

	public int getGrass() {
		return grass;
	}

	public void incGrass() {
		this.grass++;
	}

}
