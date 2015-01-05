package org.laseeb.pphpc;

import java.util.ArrayList;
import java.util.List;

public abstract class SimSynchronizer implements ISimSynchronizer {

	List<Observer> observers;
	SimEvent event;
	
	public SimSynchronizer(SimEvent event) {
		this.event = event;
		this.observers = new ArrayList<Observer>();
	}
	
	@Override
	public void registerObserver(Observer observer) {
		this.observers.add(observer);
	}

	@Override
	public void notifyObservers() {
		for (Observer o : this.observers) {
			o.update(event);
		}
	}
}
