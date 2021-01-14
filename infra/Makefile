all:
	$(MAKE) --directory .. $@
%:
	$(MAKE) --directory .. $@

install install-strip:
	$(MAKE) --directory .. $@
	cp ../contrib/signet/miner ../target/bin
	cp --recursive ../test/functional/test_framework ../target/bin
