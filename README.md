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

* [Netlogo](https://github.com/fakenmc/pphpc/tree/netlogo/netlogo)
([certified OpenABM implementation](https://www.openabm.org/model/4693/))
* [Java (parallel)](https://github.com/fakenmc/pphpc/tree/java/java)
* [OpenCL (GPU+CPU)](https://github.com/fakenmc/pphpc/tree/opencl/opencl)
(in progress)

#### Publications

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C., (2015)
Towards a standard model for research in agent-based modeling and simulation,
[PeerJ Computer Science](https://peerj.com/computer-science/),
1:e36,
http://dx.doi.org/10.7717/peerj-cs.36

* Fachada, N. (2016), Agent-Based Modeling on High Performance Computing Architectures,
PhD Thesis, Instituto Superior Técnico ([link](http://hgpu.org/papers/16552_20160915225141.pdf))

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C., (2017)
Parallelization strategies for spatial agent-based models,
[International Journal of Parallel Programming](http://www.springer.com/computer/theoretical+computer+science/journal/10766),
45(3):449-481,
http://dx.doi.org/10.1007/s10766-015-0399-9
([arXiv preprint](http://arxiv.org/abs/1507.04047))

* Fachada, N., Lopes, V.V., Martins, R.C., Rosa, A.C., (2017)
Model-independent comparison of simulation output,
[Simulation Modelling Practice and Theory](https://www.journals.elsevier.com/simulation-modelling-practice-and-theory),
72:131-149,
http://dx.doi.org/10.1016/j.simpat.2016.12.013
([arXiv preprint](http://arxiv.org/abs/1509.09174))

* Fachada, N., Rosa, A.C., (2017)
Assessing the feasibility of OpenCL CPU implementations for agent-based simulations,
[Proceedings of the 5th International Workshop on OpenCL (IWOCL 2017)](http://www.iwocl.org/),
Article No. 4,
http://doi.acm.org/10.1145/3078155.3078174

#### Licenses

Several Open Source licenses, which depend on the implementation.
