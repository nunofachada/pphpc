package org.laseeb.pphpc;

public interface IModelEventObservable {
	
	public void registerObserver(ModelEvent event, IModelEventObserver observer);

}
