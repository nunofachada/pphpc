package org.laseeb.pphpc;

public interface IWorkProvider {

	public IWork newWork(int wId);

	public int getNextToken(IWork work);
	
	public void resetWork(IWork work);

}
