/**
 * 
 */
package org.laseeb.predpreysimple;

/**
 * Generic agent class.
 * @author Nuno Fachada
 */
public abstract class Agent implements Cloneable {
	
	/* Agent energy. */
	protected int energy;
	
	/**
	 * Constructor.
	 * @param energy Initial agents' energy.
	 */
	public Agent(int energy) {
		this.energy = energy;
	}
	
	/**
	 * Returns the agents' energy.
	 * @return The agent's energy.
	 */
	public int getEnergy() {
		return energy;
	}

	/**
	 * Sets the agent energy.
	 * @param energy Value to which set the agents' energy-
	 */
	public void setEnergy(int energy) {
		this.energy = energy;
	}

	/**
	 * Decrements the agent's energy.
	 */
	public void decEnergy() {
		this.energy--;
	}
	
	/**
	 * Agent-specific actions.
	 * @param cell Cell where agent is currently in.
	 */
	protected abstract void play(Cell cell);
	
	/**
	 * Generic agent actions, consisting of:
	 * - Specific agent actions.
	 * - Reproduction.
	 * @param cell Cell where agent is currently in.
	 */
	public void doPlay(Cell cell) {
		/* Perform specific agent actions. */
		play(cell);
		/* Maybe perform reproduction. */
		reproduce(cell);
	}
	
	/**
	 * Reproduction action.
	 * @param cell Cell where agent is currently in.
	 */
	protected void reproduce(Cell cell) {
		/* Energy needs to be above threshold in order for agents to reproduce. */
		if (energy > getReproduceThreshold()) {
			/* Throw dice, see if agent reproduces. */
			if (PredPreySimple.nextInt(99) < getReproduceProbability()) {
				/* Create new agent with half of the energy of the current agent. */
				Agent agent = null;
				try {
					agent = (Agent) this.clone();
					agent.energy = this.energy / 2;
					this.energy = this.energy - agent.energy;
				} catch (CloneNotSupportedException e) {
					e.printStackTrace();
				}
				/* Put new agent in current cell. */
				cell.putAgentFuture(agent);
			}
		}
	}
	
	/**
	 * Returns the agent-specific reproduction threshold.
	 * @return Agent-specific reproduction threshold.
	 */
	protected abstract int getReproduceThreshold();
	
	/**
	 * Returns the agent-specific reproduction probability.
	 * @return Agent-specific reproduction probability.
	 */
	protected abstract int getReproduceProbability();
}
