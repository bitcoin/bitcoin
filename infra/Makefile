all:
	$(MAKE) --directory .. $@
%:
	$(MAKE) --directory .. $@

install:
	$(MAKE) --directory .. install
	cp ../contrib/signet/miner ../target/bin
	cp --recursive ../test/functional/test_framework ../target/bin
