%.capnp:
	@:
%.capnp.c++ %.capnp.h %.capnp.proxy-client.c++ %.capnp.proxy-types.h %.capnp.proxy-server.c++ %.capnp.proxy-types.c++ %.capnp.proxy.h: %.capnp
	$(AM_V_GEN) $(MPGEN_PREFIX)/bin/mpgen '$(srcdir)' '$(srcdir)' $<
