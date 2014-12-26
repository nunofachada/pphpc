package org.laseeb.pphpc;

import java.util.Collections;
import java.util.List;

public class CellPutAgentSyncSort implements CellPutAgentBehavior {

	public CellPutAgentSyncSort() {}

	@Override
	public void putAgent(List<Agent> agents, Agent agent) {
		synchronized (agents) {
			agents.add(agent);
			Collections.sort(agents);
		}
	}

}
