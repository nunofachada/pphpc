package org.laseeb.pphpc;

import java.util.HashMap;
import java.util.Map;

public abstract class AbstractWorkFactory implements IWorkFactory {

	private Map<Integer, IWorkProvider> workProviders;
	
	public AbstractWorkFactory() {
		this.workProviders = new HashMap<Integer, IWorkProvider>();
	}
	
	@Override
	public IWorkProvider getWorkProvider(int workSize, IController controller) {
		if (!this.workProviders.containsKey(workSize)) {
			synchronized (this) {
				if (!this.workProviders.containsKey(workSize)) {
					IWorkProvider workProvider = this.doGetWorkProvider(workSize, controller);
					this.workProviders.put(workSize, workProvider);
				}
			}
		}
		return this.workProviders.get(workSize);
	}

	protected abstract IWorkProvider doGetWorkProvider(int workSize, IController controller);

}
