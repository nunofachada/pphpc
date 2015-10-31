# PPHPC: a standard model for research in agent-based modeling and simulation

#### What does PPHPC mean?

Predator-Prey for High-Performance Computing

#### What is the purpose of this model?

The purpose of PPHPC is to serve as a standard model for studying and evaluating 
spatial agent-based model (SABM) implementation strategies. It is a realization 
of a predator-prey dynamic system, and captures important characteristics of 
SABMs, such as agent movement and local agent interactions. The model can be 
implemented using substantially different approaches that ensure statistically 
equivalent qualitative results. Implementations may differ in aspects such as 
the selected system architecture, choice of programming language and/or 
agent-based modeling framework, parallelization strategy, random number 
generator, and so forth. By comparing distinct PPHPC implementations, valuable 
insights can be obtained on the computational and algorithmical design of SABMs
in general.

#### Currently available implementations

* [Netlogo](https://github.com/FakenMC/pphpc/tree/netlogo)
([certified OpenABM implementation](https://www.openabm.org/model/4693/))
* [Java (parallel)](https://github.com/FakenMC/pphpc/tree/java)
* [OpenCL (GPU+CPU)](https://github.com/FakenMC/pphpc/tree/opencl) (in progress)

#### Publications

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C.,
Towards a standard model for research in agent-based modeling and simulation, 
under review, 2015 
([PeerJ preprint](https://peerj.com/preprints/1440/))

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C., 
Parallelization strategies for spatial agent-based models, accepted for
publication in the [International Journal of Parallel Programming](http://www.springer.com/computer/theoretical+computer+science/journal/10766), 
2015 
([arXiv preprint](http://arxiv.org/abs/1507.04047))

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C., 
Model-independent comparison of simulation output, under review, 2015 
([arXiv preprint](http://arxiv.org/abs/1509.09174))

#### Licenses

Several Open Source licenses, which depend on the implementation.

