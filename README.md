Introduction
================================================================================================================
Mammut (MAchine Micro Management UTilities) is a set of functions for the management of a local or a remote
machine. It is structured as a set of modules, each managing a specific functionality (e.g. CPU frequencies,
topology analysis, etc...). 
Currently, the following modules are present:

+ Topology: Allows the navigation and the management of the CPU, Physical Cores and Virtual Cores
  of the machine.
+ CPUFreq: Allows to read and change the frequency and the governors of the Physical Cores.
+ Energy: Allows to read the energy consumption of the CPUs.

To manage a remote machine, ```mammut-server``` must run on that machine.

Installation
================================================================================================================
Mammut depends on [Google's Protocol Buffer](http://code.google.com/p/protobuf/) library. This library is
used for the communication with ```mammut-server``` when the management of a remote machine is required. 

After installing [Google's Protocol Buffer](http://code.google.com/p/protobuf/), fetch the framework typing:

```
$ git clone git://github.com/DanieleDeSensi/Mammut.git
$ cd ./Mammut/mammut
```

Compile it with:

```
$ make
```

After that, install it with

```
$ make install
```

Server usage
================================================================================================================
To manage a remote machine, ```mammut-server``` must run on that machine. It accepts the following parameters:

+ [Mandatory] --tcpport: TCP port used by the server to wait for remote requests.
+ [Optional] --cpufreq: Activates cpufreq module.
+ [Optional] --topology: Activates topology module.
+ [Optional] --energy: Activates energy module.

If a module is not activated, any attempt of a client to use that module will fail.

