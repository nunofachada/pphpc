package org.laseeb.pphpc;

import java.util.Collections;
import java.util.List;

public class CellFutureIsNowPostSort implements CellFutureIsNowPostBehavior {

	public CellFutureIsNowPostSort() {}

	@Override
	public void futureIsNowPost(List<Agent> agents) {
		
		Collections.sort(agents);

	}

}
