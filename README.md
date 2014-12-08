# PORTFWD #

----------

PORTFWD is a simple TCP port forwarding / tunneling utility for Windows. Two instances of PORTFWD are required for tunneling mode and the data between the two will be XOR munged.

PORTFWD can be built as a standalone application, DLL, or static library to be used in other applications.

## Platforms ##

PORTFWD builds using VS2013 and is supported on all x86, x64, and ARM Windows versions (binaries provided)

## Usage ##

        PORTFWD Usage:

        portfwd.exe -l <local port> -a <remote address> -p <remote port> -t(unnel mode)


## Example ##

Simple port forwarding:

    portfwd.exe -l 6666 -a localhost -p 7777


Tunneling - First instance:

    portfwd.exe -l 6666 -a localhost -p 7777 -t


Tunneling - Second instance

	portfwd.exe -l 7777 -a localhost -p 8888 -t