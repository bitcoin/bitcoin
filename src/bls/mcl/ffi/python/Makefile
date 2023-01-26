include ../../common.mk

SHE384_256_SLIB=libmclshe384_256.$(LIB_SUF)

she_test: ../../lib/$(SHE384_256_SLIB)
	cd ../../lib && env LD_LIBRARY_PATH=./ python3 ../ffi/python/she.py

she_test_g1only: ../../lib/$(SHE384_256_SLIB)
	cd ../../lib && env LD_LIBRARY_PATH=./ python3 ../ffi/python/she.py g1only

../../lib/$(SHE384_256_SLIB):
	make -C ../../ lib/$(SHE384_256_SLIB)
