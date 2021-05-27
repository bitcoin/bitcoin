This directory contains interfaces and implementations that isolate the
rest of the package from platform details.

Code in the rest of the package includes "port.h" from this directory.
"port.h" in turn includes a platform specific "port_<platform>.h" file
that provides the platform specific implementation.

See port_stdcxx.h for an example of what must be provided in a platform
specific header file.

