package org.laseeb.pphpc;

import java.util.List;

public class CellPutAgentAsync implements CellPutAgentBehavior {

	public CellPutAgentAsync() {}

	@Override
	public void putAgent(List<Agent> agents, Agent agent) {
		agents.add(agent);
	}

}
