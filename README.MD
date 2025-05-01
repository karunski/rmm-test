# Introduction

Summary of how I approached each of the criteria in the task:

> Run on either Windows, Linux, or macOS (pick just one)

This service targets Linux.

> Run continuously – when launched, program should keep running until user stops it

My approach here is to just have the service enter an event loop and run continously to respond to
events received.  

It does not currently do anything special to behave as a daemon, and it just relies on the default signal handler behavior to stop when a signal like SIGINT or SIGTERM is received.

TODO: create systemd unit example

> Constantly monitor the CPU OR Total System Memory (RAM) and output them to files. You only have to 
> pick one but if you feel inclined, implement both.

Program metrics are written to the $install_prefix/var/stats directory.  They are just concatenated to the log files there.

This program defines these metrics:

 * I choose to define the CPU metric as "% of total CPU time not spent idle". It seems to be a 
   reasonable approximation of how busy the CPU is, without getting into the accounting of the various 
   running CPU states Linux provides.
 * For Memory I report the "MemAvailable" stat:  This seems like a good high-level summary of memory
   pressure on the host.  

> The program should be able to run custom scripts. One idea is for the agent to watch a specific location in the File System for the existence of the script file and run from there. Output any results of the Script to another file. For simplicity, assume that there will only be one file in the folder at a time.

This is implemented by providing a "drop box" for scripts: $install_prefix/var/scripts.  Files copied or
moved into this directory are run.

 * Any scripts have to be self-launching by providing a `#!/path/to/interpreter` she-bang header
 * The script must be executable by the UID/GID running this utility.

> Bonus: Implement the monitoring and script running asynchronous so the main thread doesn’t block.

This program is implemented as a single-threaded event loop: nothing blocks the main (and only) thread.

# Building

## Tools used / required:

 * CMake >= 3.28
 * Ninja build tool (other cmake-supported tools are possible, but the provided cmake preset assumes Ninja)
 * vcpkg manager; set up as described in it's [documentation](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell)
 * Some sort of reasonably modern version of GCC g++-- this program targets the C++17 standard.
