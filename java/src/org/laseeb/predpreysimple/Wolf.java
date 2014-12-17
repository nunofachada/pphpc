package org.laseeb.predpreysimple;

import java.util.Iterator;

/**
 * Wolf class.
 * @author Nuno Fachada
 *
 */
public class Wolf extends Agent {
	
	/**
	 * Constructor.
	 * @param energy Initial agents' energy.
	 */
	public Wolf(int energy) {
		super(energy);
	}

	@Override
	/**
	 * @see Agent
	 */
	protected void play(Cell cell) {
		/* Iterate over agents in this cell. */
		Iterator<Agent> agents = cell.getAgents();
		while (agents.hasNext()) {
			/* Get next agent. */
			Agent agent = agents.next();
			/* Check if agent is sheep. */
			if (agent instanceof Sheep) {
				/* Eat sheep (only one please). */
				if (agent.getEnergy() > 0) {
					this.setEnergy(this.getEnergy() + PredPreySimple.WOLVES_GAIN_FROM_FOOD);
					cell.removeAgent(agent);
					agent.setEnergy(0);
					break;
				}
			}
		}
	}
	
	@Override
	/**
	 * @see Agent
	 */
	protected int getReproduceProbability() {
		return PredPreySimple.WOLVES_REPRODUCE_PROB;
	}
	
	@Override
	/**
	 * @see Agent
	 */
	protected int getReproduceThreshold() {
		return PredPreySimple.WOLVES_REPRODUCE_THRESHOLD;
	}

}
