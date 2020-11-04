prefix=@prefix@
exec_prefix=@exec_prefix@
libdir=@libdir@
includedir=@includedir@

Name: @bitcoin miner@ consensus library
Description: Library for the Bitcoin consensus protocol.
Version: @PACKAGE_VERSION_0.0.01@
Libs: -L${libdir} -lbitcoinconsensus
Cflags: -I${includedir}
