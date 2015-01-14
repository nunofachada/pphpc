package org.laseeb.pphpc;

public enum ViewType {
	
	/**
	 * View only reads model values.
	 */
	PASSIVE,
	
	/**
	 * View controls the simulation using an IController object.
	 */
	ACTIVE,
	
	/**
	 * View controls the simulation using an IController object, and must do it
	 * exclusively (i.e. no more ACTIVE views are allowed).
	 */
	ACTIVE_EXCLUSIVE

}
