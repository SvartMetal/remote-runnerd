# Remote runner daemon #
Simple daemon for running commands remotely.
Based on boost asio which provides asynchronous io.
Daemon reads configuration file and runs only allowed commands.
By default configuration file path is `/etc/remote-runnerd.conf`.
You can configure daemon in `./src/settings.h` file.
By default daemon use `tcp` port `12345` and local socket with address `/tmp/simple-telnetd`.

## Prerequisites ##
Daemon can be built on unix-like systems.
Dependencies:
```
boost   >= 1.55
g++     >= 4.8
```
## Building remote runner daemon ##
Clone this repository, `cd` into it and type `make`.
To clean use `make clean`.

## Configuration file format ##
Configuration file consists of 'commands' and 'programs'.
Configuration file example:
```
ls      /bin/ls
pwd     /bin/pwd
```

## Launching remote runner daemon ##
You can use `./build/remote-runnerd <timeout>` or simply
`make run` (this will run daemon with `timeout = 5`).

Daemon returns result to the user in format:
```
<Execution status>
*** STDOUT ***
*** STDERR ***
```

