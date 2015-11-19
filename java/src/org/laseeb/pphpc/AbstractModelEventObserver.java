/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

/**
 * Model event observers which extend this class instead of directly implementing
 * {@link IModelEventObserver} only override event specific methods of interest
 * instead of implementing the {@link IModelEventObserver#update(ModelEvent)} method,
 * and check for all event possibilities.  
 * 
 * @author Nuno Fachada
 */
public abstract class AbstractModelEventObserver implements IModelEventObserver {

	/**
	 * @see IModelEventObserver#update(ModelEvent)
	 */
	@Override
	public void update(ModelEvent event) {
		switch (event) {
			case START:
				this.updateOnStart();
				break;
			case UNPAUSE:
				this.updateOnUnpause();
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
	
	/**
	 * Method invoked on start events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnStart() {}
	
	/**
	 * Method invoked on unpause events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnUnpause() {}

	/**
	 * Method invoked on new iteration events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnNewIteration() {}

	/**
	 * Method invoked on pause events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnPause() {}

	/**
	 * Method invoked on stop events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnStop() {}

	/**
	 * Method invoked on exception events. By default it does nothing unless
	 * a concrete implementation overrides it.
	 */
	protected void updateOnException() {}

}
