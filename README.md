# Remote runner daemon #
Simple daemon for running commands remotely.
Based on boost asio which provides asynchronous io.
Daemon reads config file and runs only allowed commands.
You can configure it in `./src/settings.h` file.
By default daemon uses `tcp` port `12345` and local socket with address `/tmp/simple-telnetd`

## Prerequisites ##
Daemon can be built on unix-like systems.
Dependencies:
```
boost >= 1.55
g++ >= 4.8
```

## Building remote runner daemon ##
Clone this repository, `cd` into it and type `make`.
To clean use `make clean`.

## Launching remote runner daemon ##
You can use `./build/remote-runnerd <timeout>` or simply
`make run` (this will run daemon with timeout = 5).

