package org.laseeb.pphpc;

public interface ISimSynchronizer {
	
	public void syncNotify();
	
	public void registerObserver(Observer observer);

	public void notifyObservers();
}
