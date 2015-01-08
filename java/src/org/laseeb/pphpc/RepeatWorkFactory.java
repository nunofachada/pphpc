package org.laseeb.pphpc;

import com.beust.jcommander.Parameters;

@Parameters(commandNames = {"repeat"}, commandDescription = "Repeatable work command")
public class RepeatWorkFactory extends AbstractEqualWorkFactory {
	
	final private String commandName = "repeat";

	@Override
	public IWorkProvider doGetWorkProvider(int workSize, IController controller) {
		return new EqualWorkProvider(this.numThreads, workSize);
	}

	
	@Override
	public ICellPutAgentStrategy createPutNewAgentStrategy() {
		return new CellPutAgentSyncSort();
	}

	@Override
	public ICellPutAgentStrategy createPutExistingAgentStrategy() {
		return new CellPutAgentSyncSort();
	}

	@Override
	public String getCommandName() {
		return this.commandName;
	}


}
