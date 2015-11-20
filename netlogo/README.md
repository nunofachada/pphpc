## NetLogo implementation of the PPHPC model

### Running the model manually using the NetLogo GUI

1. Launch NetLogo

2. Open the `pphpc.nlogo` file (File => Open)

3. Edit some of the model parameters

4. Click `Setup` to initialize the simulation (iteration 0)

5. Click `Go` to run the simulation

### Running the model with standard parameters using the NetLogo GUI

1. Launch NetLogo

2. Open the `pphpc.nlogo` file (File => Open)

3. Open the BehaviorSpace dialog (Tools => BehaviorSpace)

4. Select one of the standard parameters (discussed in detail
[here](https://peerj.com/articles/cs-36/)), click `Run`

5. Select desired output formats, set 
`Simultaneous runs in parallel` to 1, click `Ok`

### Running the model with standard parameters using the command line

The following command assumes that `$NLDIR` specifies the location where
NetLogo is installed, and that `$PPDIR` contains the location of the
`pphpc.nlogo` file.

```
java -Xmx2048m -Dfile.encoding=UTF-8 -cp ${NLDIR}/NetLogo.jar org.nlogo.headless.Main --model ${PPDIR}/pphpc.nlogo --threads 1 --experiment ex100v1 --table out.csv
```

The `-Xmx2048m` option sets the maximum usable memory to 2GB. In this
case, the simulation is performed with the standard parameters defined
in the `ex100v1` BehaviorSpace experiment. On Windows the command is
slightly different:

```
java -Xmx2048m -Dfile.encoding=UTF-8 -cp %NLDIR%\NetLogo.jar org.nlogo.headless.Main --model %PPDIR%\pphpc.nlogo --threads 1 --experiment ex100v1 --table out.csv
```

where `%NLDIR%` and `%PPDIR%` contain the location of NetLogo and of the
`pphpc.nlogo` file, respectively.

### Running the model with custom parameters using the command line

It is possible to specify custom parameters using the command line by
passing a XML setup file, as described in the 
[BehaviorSpace](https://ccl.northwestern.edu/netlogo/docs/behaviorspace.html)
chapter of the NetLogo manual, under section 
*Setting up experiments in XML*.

For example, if the setup file is named `myparams.xml` and one of the
specified experiments is called `params1`, the PPHPC model would be
invoked with the following command:

```
java -Xms2048m -Dfile.encoding=UTF-8 -cp ${NLDIR}/NetLogo.jar org.nlogo.headless.Main --model ${PPDIR}/pphpc.nlogo --setup-file ./myparams.xml --threads 1 --experiment params1 --table out.csv
```

On Windows it would be:


```
java -Xms2048m -Dfile.encoding=UTF-8 -cp %NLDIR%\NetLogo.jar org.nlogo.headless.Main --model %PPDIR%\pphpc.nlogo --setup-file myparams.xml --threads 1 --experiment params1 --table out.csv
```
