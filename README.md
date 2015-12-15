Introduction
================================================================================================================
**ATTENTION**: This library is still in development and its interface may change, as well as the signatures of some
member functions.

Mammut (MAchine Micro Management UTilities) is a set of functions for the management of a local or a remote
machine, providing an object oriented abstraction of features normally provided by means of sysfs files or CPU registries. 
It is structured as a set of modules, each managing a specific functionality (e.g. CPU frequencies,
topology analysis, energy counters reading, etc...). 
Mammut also provide the possibility to manage remote machines by using a client server mechanism. 

Currently, the following modules are present:

+ [Topology](./mammut/topology): Allows the navigation and the management of the CPU, Physical Cores and Virtual Cores
  of the machine. For example, it is possible to force cores into deepest idle states, to check their utilisation 
  percentage, to read their voltage or to visit the topology tree.
+ [CPUFreq](./mammut/cpufreq): Allows to read and change the frequency and the governors of the Physical Cores.
+ [Energy](./mammut/energy): Allows to read the energy consumption of the machine. Currently, the following architectures are supported:
	- **Intel SandyBridge, IvyBridge and Haswell**. For these architectures, it is possible to read energy consumption of each CPU, 
	  of the cores on the CPU, of the DRAM controller and of the integrated graphic card. On some of these machines DRAM or 
	  integrated graphic counters may not be available.
      To use the energy counters, the  ```msr ``` module must be loaded. A  ```sudo modprobe msr ``` should be enough to let it work.
	- **[Odroid](http://www.hardkernel.com/main/main.php) machines**. For these architectures, it is possible to read the total energy consumption if a 
	  [SmartPower](http://odroid.com/dokuwiki/doku.php?id=en:odroidsmartpower) is connected to the machine.
	- **Other machines** with a power supply voltage **between 3.0V and 5.25V**. For these machines, it is possible to
	  read the total energy consumption if a SmartPower is connected to the machine, similarly to the previous point.
+ [Task](./mammut/task): Allows to manage processes and threads. For example, it is possible to map threads on 
  physical or virtual cores, to change their priority or to read statistics about them (e.g. cores' utilisation 
  percentage).

The documentation for each module can be found on the corresponding folder.

To manage a remote machine, ```mammut-server``` must run on that machine. 
From the user perspective, the only difference between managing a local or a 
remote machine concerns the module initialization. For example,
to get a local instance of the topology module:

```C++
Mammut m;
Topology* t = m.getInstanceTopology();
```

and to get a remote instance:

```C++
Mammut m(new CommunicatorTcp(remoteMachineAddress, remoteMachinePort));
Topology* t = m.getInstanceTopology();
```

After that, all the other calls to the module are exactly the same in both cases.

Similar ```getInstance*()``` calls are available for the other modules.

Demo
----------------------------------------------------------------------------------------------------------------
Some demonstrative example can be found in folder [demo](./demo).

Installation of Mammut
================================================================================================================
Fetch the framework by typing:
```
$ git clone git://github.com/DanieleDeSensi/Mammut.git
$ cd ./Mammut
```

Then compile it with:
```
$ make
```

After that, install it with

```
$ make install
```

You can specify the installation path by modifying the ```MAMMUT_PATH``` 
variable in the [Makefile](./Makefile). The standard subfolders will be 
then used for the installation (```lib```, ```include```, ```bin```).

Please read the notes (end of this document) before using the library. 

Installation of Mammut with remote management support
================================================================================================================
To interact with ```mammut-server``` in order to manage a remote machine, 
Mammut uses [Google's Protocol Buffer Library](https://github.com/google/protobuf) 
library. For this reason, if you also need to manage remote machines,
you need to install [Google's Protocol Buffer Library](https://github.com/google/protobuf).

After the installation, you can install a remote-enabled Mammut by following 
the same procedure shown at the previous point.
The only exception is that now you need to compile the library with
```make remote``` instead of ```make```.

If ```Google's Protocol Buffer Library``` has been installed into a non standard
path, it is possible to specify this path by modifying the ```PROTOBUF_PATH``` 
variable in the [Makefile](./Makefile).

Server usage
================================================================================================================
To manage a remote machine, ```mammut-server``` must run on that machine. It accepts the following parameters:

+ [Mandatory] --tcpport: TCP port used by the server to wait for remote requests.
+ [Optional] --verbose [level]: Prints information during the execution.
+ [Optional] --cpufreq: Activates cpufreq module.
+ [Optional] --topology: Activates topology module.
+ [Optional] --energy: Activates energy module.
+ [Optional] --all: Activates all the modules.

If a module is not activated, any attempt of a client to use that module will fail.

Notes
================================================================================================================
+ Many of the feature provided by this library can only be used by privileged users.
+ The remote support is still an experimental feature and some functionalities 
available on the local support may not be available remotely.
