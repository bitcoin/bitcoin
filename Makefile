FLAGS_COMMON:=-Wall
FLAGS_PROD:=-DNDEBUG -g0 -O3 -march=native
FLAGS_DEBUG:=-DVERIFY_MAGNITUDE -ggdb3 -O1
FLAGS_TEST:=-DVERIFY_MAGNITUDE -ggdb3 -O3 -march=native

SECP256K1_FILES := num.h   field.h   group.h   ecmult.h   ecdsa.h \
                   num.cpp field.cpp group.cpp ecmult.cpp ecdsa.cpp \

ifndef CONF
CONF := gmp
endif

ifeq ($(CONF), openssl)
FLAGS_CONF:=-DUSE_NUM_OPENSSL -DUSE_FIELDINVERSE_BUILTIN
LIBS:=-lcrypto
SECP256K1_FILES := $(SECP256K1_FILES) num_openssl.h num_openssl.cpp
else
ifeq ($(CONF), gmp)
FLAGS_CONF:=-DUSE_NUM_GMP
LIBS:=-lgmp
SECP256K1_FILES := $(SECP256K1_FILES) num_gmp.h num_gmp.cpp 
endif
endif

all: *.cpp *.h
	+make CONF=openssl all-openssl
	+make CONF=gmp     all-gmp

clean:
	+make CONF=openssl clean-openssl
	+make CONF=gmp     clean-gmp

bench-any: bench-$(CONF)
tests-any: tests-$(CONF)

all-$(CONF): bench-$(CONF) tests-$(CONF)

clean-$(CONF):
	rm -f bench-$(CONF) tests-$(CONF) obj/secp256k1-$(CONF).o

obj/secp256k1-$(CONF).o: $(SECP256K1_FILES)
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) secp256k1.cpp -c -o obj/secp256k1-$(CONF).o

bench-$(CONF): obj/secp256k1-$(CONF).o bench.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_PROD) $(FLAGS_CONF) obj/secp256k1-$(CONF).o bench.cpp $(LIBS) -o bench-$(CONF)

tests-$(CONF): $(SECP256K1_FILES) tests.cpp
	$(CXX) $(FLAGS_COMMON) $(FLAGS_TEST) $(FLAGS_CONF) tests.cpp $(LIBS) -o tests-$(CONF)
