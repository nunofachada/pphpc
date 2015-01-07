package org.laseeb.pphpc;

public interface IWorkFactory {

	public IWorkProvider createWorkProvider();
	
	public ICellPutAgentStrategy createPutNewAgentStrategy();

	public ICellPutAgentStrategy createPutExistingAgentStrategy();
	
	public IController createSimController(IModel model);

	public IGlobalStats createGlobalStats(int iters);
	
	public int getNumWorkers();
	
	public String getCommandName();
	
}
