package org.laseeb.pphpc;

import java.util.concurrent.atomic.AtomicInteger;

public abstract class AbstractWorkProvider implements IWorkProvider {

	private AtomicInteger wIdGenerator;
	
	public AbstractWorkProvider() {
		this.wIdGenerator = new AtomicInteger(0);
	}

	DELETE THIS?
	
	@Override
	public IWork newWork(int workSize) {

		/* Determine unique work ID. */
		int wId = this.wIdGenerator.getAndIncrement();
		
		/* Perform specific work provider initialization. */
		return doRegisterWork(wId, workSize);
	}

	protected abstract IWork doRegisterWork(int wId, int workSize);


}
