ifeq ($(findstring MINGW64,$(shell uname -s)),MINGW64)
  # cgo accepts not '/c/path' but 'c:/path'
  PWD=$(shell pwd|sed s'@^/\([a-z]\)/@\1:/@')
else
  PWD=$(shell pwd)
endif
PATH_VAL=$$PATH:$(COMMON_LIB_PATH) LD_LIBRARY_PATH=$(COMMON_LIB_PATH) DYLD_LIBRARY_PATH=$(COMMON_LIB_PATH) CGO_LDFLAGS="-L../../lib" CGO_CFLAGS="-I$(PWD)/../../include -I$(PWD)/../../../mcl/include"
../../lib/libbls384_256.a:
	$(MAKE) -C ../.. clean
	$(MAKE) -C ../.. minimized_static

./sample: sample.go ../../lib/libbls384_256.a
	env CGO_CFLAGS="-I$(PWD)/../../include -I$(PWD)/../../../mcl/include" CGO_LDFLAGS="-L$(PWD)/../../lib" go build -tags bn384_256 sample.go

test: ./sample
	./sample

update:
	patch -o - -p0 ../../../mcl/ffi/go/mcl/mcl.go < mcl.patch > bls/mcl.go
update_patch:
	-diff -upN ../../../mcl/ffi/go/mcl/mcl.go bls/mcl.go > mcl.patch

clean:
	$(RM) ./sample
