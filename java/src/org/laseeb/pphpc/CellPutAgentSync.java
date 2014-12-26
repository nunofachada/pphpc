package org.laseeb.pphpc;

import java.util.List;

public class CellPutAgentSync implements CellPutAgentBehavior {

	public CellPutAgentSync() {}

	@Override
	public void putAgent(List<Agent> agents, Agent agent) {
		synchronized (agents) {
			agents.add(agent);
		}
	}

}
