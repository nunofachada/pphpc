package org.laseeb.pphpc;

public interface Observable {

	public void registerObserver(SimEvent event, Observer observer);
	
	public void notifyObservers(SimEvent event);
	
}
