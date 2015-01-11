package org.laseeb.pphpc;

public abstract class AbstractModelEventObserver implements IModelEventObserver {

	@Override
	public void update(ModelEvent event) {
		switch (event) {
			case START:
				this.updateOnStart();
				break;
			case CONTINUE:
				this.updateOnContinue();
				break;
			case NEW_ITERATION:
				this.updateOnNewIteration();
				break;
			case PAUSE:
				this.updateOnPause();
				break;
			case STOP:
				this.updateOnStop();
				break;
			case EXCEPTION:
				this.updateOnException();
				break;
		}
	}
	
	protected void updateOnStart() {}
	protected void updateOnContinue() {}
	protected void updateOnNewIteration() {}
	protected void updateOnPause() {}
	protected void updateOnStop() {}
	protected void updateOnException() {}

}
