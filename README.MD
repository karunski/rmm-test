# Introduction

Summary of how I approached each of the criteria in the task:

> Run on either Windows, Linux, or macOS (pick just one)

This service targets Linux.  I chose that as a default because the interface for fetching system stats 
is as simple as reading entries in the /proc filesystem.  

The basic structure of the program will work on Mac and Windows; but those platforms would need different implementations of the CPUSampler, MemAvailable, and ScriptMonitor modules. 

> Run continuously – when launched, program should keep running until user stops it

My approach here is to just have the service enter an event loop and run continously to respond to
events received.  

It does not currently do anything special to behave as a daemon, and it just relies on the default signal handler behavior to stop when a signal like SIGINT or SIGTERM is received.

For demonstration purposes I would just run the 'rmm' command in a terminal window to observe its operation and validate its
functionality.  However if desired it's easy to turn into a transient 'service' via systemd-run. For example:

```
systemd-run build/Debug/rmm
```

> Constantly monitor the CPU OR Total System Memory (RAM) and output them to files. You only have to 
> pick one but if you feel inclined, implement both.

Program metrics are written to the build/var/stats directory.  They are just concatenated to the log files there.

This program defines these CPU and memory metrics:

 * I choose to define the CPU metric as "% of total CPU time not spent idle". It seems to be a 
   reasonable approximation of how busy the CPU is, without getting into the accounting of the various 
   running CPU states Linux provides.
 * For Memory I report the "MemAvailable" stat:  This seems like a good high-level summary of memory
   pressure on the host.  

> The program should be able to run custom scripts. One idea is for the agent to watch a specific location in the File System for the existence of the script file and run from there. Output any results of the Script to another file. For simplicity, assume that there will only be one file in the folder at a time.

This is implemented by providing a "drop box" for scripts: build/var/scripts.  Files copied or
moved into this directory are run.

 * Any scripts have to be self-launching by providing a `#!/path/to/interpreter` shebang header
 * The script must be executable by the UID/GID running this utility.

The script's output is written to build/var/script_output/$SCRIPT_BASE_NAME.{out,err}.

Scripts are run as soon as they are detected, via a filesystem event watching mechanism.  Any script detected is immediately executed. 

Example:

```
chmod +x hello.sh
cp hello.sh build/var/scripts
```

In the console we can see that the script was run:

```
Script completed: "/home/kevin/src/rmm/build/var/scripts/hello.sh", exit code: 99
```

> Bonus: Implement the monitoring and script running asynchronous so the main thread doesn’t block.

This program is implemented as a single-threaded event loop: nothing blocks the main (and only) thread.  

>The program should be resilient enough so that it can run long term without crashing or consuming extra CPU and memory; however, use proper discretion when working on this assessment so that it does not consume more of your time than needed.

I let the program run for a day; and it's holding steady at a ~100KB of RSS used.  It doesn't use
a measurable amount of CPU time.  It lacks log rotation for the stat files it writes; so don't leave
it running indefinitely or it will (very, very slowly) fill its filesystem.

# Building

## Tools used / required:

 * CMake >= 3.28
 * Ninja build tool (other cmake-supported tools are possible, but the provided cmake preset assumes Ninja)
 * vcpkg manager; set up as described in it's [documentation](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell)
 * Some sort of reasonably modern version of GCC g++-- this program targets the C++17 standard.

## Configuring and Building

If you setup a "default" CMake preset, as described in the vcpkg documentation:

```
cmake --preset default
```

Otherwise; the 'vcpkg' preset value defined in this projects CMakePresets.json might work, as long as VCPKG_ROOT is defined.

To run the compile:

```
cmake --build build --config {Debug|Release}
```

The resulting binary will be in `build/{Debug|Release}/rmm`