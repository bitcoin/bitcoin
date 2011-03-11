Project: miniupnp
Project web page: http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
Author: Thomas Bernard
Copyright (c) 2005-2009 Thomas Bernard
This software is subject to the conditions detailed in the
LICENSE file provided within this distribution.

For the comfort of Win32 users, bsdqueue.h is included in the distribution.
Its licence is included in the header of the file.
bsdqueue.h is a copy of the sys/queue.h of an OpenBSD system.

* miniupnp Client *

To compile, simply run 'gmake' (could be 'make' on your system).
Under win32, to compile with MinGW, type "mingw32make.bat".
The compilation is known to work under linux, FreeBSD,
OpenBSD, MacOS X, AmigaOS and cygwin.
The official AmigaOS4.1 SDK was used for AmigaOS4 and GeekGadgets for AmigaOS3.

To install the library and headers on the system use :
> su
> make install
> exit

alternatively, to install in a specific location, use :
> INSTALLPREFIX=/usr/local make install

upnpc.c is a sample client using the libminiupnpc.
To use the libminiupnpc in your application, link it with
libminiupnpc.a (or .so) and use the following functions found in miniupnpc.h,
upnpcommands.h and miniwget.h :
- upnpDiscover()
- miniwget()
- parserootdesc()
- GetUPNPUrls()
- UPNP_* (calling UPNP methods)

Note : use #include <miniupnpc/miniupnpc.h> etc... for the includes
and -lminiupnpc for the link

Discovery process is speeded up when MiniSSDPd is running on the machine.

* Python module *

you can build a python module with 'make pythonmodule' 
and install it with 'make installpythonmodule'.
setup.py (and setupmingw32.py) are included in the distribution.


Feel free to contact me if you have any problem :
e-mail : miniupnp@free.fr

If you are using libminiupnpc in your application, please
send me an email !


