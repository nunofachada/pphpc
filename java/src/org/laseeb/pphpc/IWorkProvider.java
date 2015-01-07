package org.laseeb.pphpc;

public interface IWorkProvider {

	public IWork newWork(int wId, int workSize);

	public int getNextToken(IWork work);
	
	public void resetWork(IWork work);

}
