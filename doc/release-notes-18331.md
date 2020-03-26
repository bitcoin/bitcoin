Build System
------------

The source code archives that are provided with gitian builds no longer contain
any autotools artifacts. Therefore, to build from such source a user
should run the `./autogen.sh` script from the root of the unpacked archive.
This implies that `autotools` and other required packages are installed on
user's system.
