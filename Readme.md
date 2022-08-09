## Overview

This repository stores a new approach for exceptions handling in MPI applications. The goals are to
1. report any faulty state to the user in a nicely formatted way by just a single rank
1. ensure the application will never deadlock, 
1. propose a simple interface and ensure interoperability with other C/C++ libraries.


## Compilation

The code can be compiled with OpenMPI + GCC or Intel MPI. 

```bash
make
```


## Usage

Open the source code and select the example. Then compile the code and run it

```bash
mpirun ./MPIErrorChecker 
```
