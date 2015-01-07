package org.laseeb.pphpc;

public interface IWorkFactory {

	public IWorkProvider createWorkProvider(int workSize);
	
	public ICellPutAgentStrategy createPutNewAgentStrategy();

	public ICellPutAgentStrategy createPutExistingAgentStrategy();
	
	public IController createSimController(IModel model);

	public IGlobalStats createGlobalStats();
}
