package org.laseeb.predpreymulti;

import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 
 * @author Nuno Fachada
 * Grid cell, which contains grass and agents.
 */
public class Cell {
	
	/* Structure where to keep current agents. */
	private Set<Agent> agents;
	/* Structure where to put future agents. */
	private ConcurrentHashMap<Integer, Agent> futureAgents;
	/* Structure where to put agents to be removed. */
	private HashSet<Agent> agentsToRemove;
	
	/* Grass counter. */
	private int grass;
	
	/**
	 * Constructor.
	 */
	public Cell() {
		/* Initialize agent keeping structures. */
		agents = new HashSet<Agent>();
		futureAgents = new ConcurrentHashMap<Integer, Agent>(16, 0.75f, PredPreyMulti.NUM_THREADS);
		agentsToRemove = new HashSet<Agent>();
	}
	
	/**
	 * In the future, put new agent in this cell.
	 * @param agent Agent to put in cell in the future.
	 */
	public void putAgentFuture(Agent agent) {
		if (agent.getEnergy() > 0)
			futureAgents.put(agent.hashCode(), agent);
	}
	
	/**
	 * Put new agent in this cell now. Only used for initialization.
	 * @param agent Agent to put in cell now.
	 */
	public void putAgentNow(Agent agent) {
		if (agent.getEnergy() > 0)
			agents.add(agent);
	}

	/**
	 * Make future agents the current agents.
	 */
	public void futureIsNow() {
		agents.clear();
		agents.addAll(futureAgents.values());
		futureAgents.clear();
	}
	
	/**
	 * Remove agents to be removed.
	 */
	public void removeAgentsToBeRemoved() {
		agents.removeAll(agentsToRemove);
		agentsToRemove.clear();
	}
	
	/**
	 * Returns an iterator over agents in this cell.
	 * @return Iterator for agents in this cell.
	 */
	public Iterator<Agent> getAgents() {
		return Collections.synchronizedSet(agents).iterator();
	}
	
	/**
	 * Remove agent from this cell.
	 * @param agent Agent to remove from cell.
	 */
	public void removeAgent(Agent agent) {
		agentsToRemove.add(agent);
	}
	
	/**
	 * Return grass counter value.
	 * @return Grass counter value.
	 */
	public int getGrass() {
		return grass;
	}
	
	/**
	 * Eat grass.
	 */
	public void eatGrass() {
		grass = PredPreyMulti.GRASS_RESTART;
	}
	
	/**
	 * Decrement grass counter.
	 */
	public void decGrass() {
		grass--;
	}
	
	/**
	 * Set grass counter to a specific value.
	 * @param grass Value to set grass counter.
	 */
	public void setGrass(int grass) {
		this.grass = grass;
	}

}
