package org.laseeb.pphpc;

public interface IView extends IModelEventObserver {
	
	public void init(IModel model, IController controller, PredPrey pp);

}
