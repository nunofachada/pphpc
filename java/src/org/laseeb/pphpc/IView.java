package org.laseeb.pphpc;

public interface IView extends IModelEventObserver {
	
	public void init(IModelQuerier model, IController controller, PredPrey pp);
	
	public ViewType getType();

}
