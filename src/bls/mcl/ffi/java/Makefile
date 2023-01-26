TOP_DIR=../..
include $(TOP_DIR)/common.mk
ifeq ($(UNAME_S),Darwin)
  JAVA_INC_DIR?=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/JavaVM.framework/Headers/
  JAVA_INC?=-I$(JAVA_INC_DIR)
  CFLAGS+=$(JAVA_INC)/darwin
else
  JAVA_INC_DIR?=/usr/lib/jvm/default-java/include
  JAVA_INC?=-I$(JAVA_INC_DIR)
#JAVA_INC=-I/usr/lib/jvm/java-7-openjdk-amd64/include
  CFLAGS+=$(JAVA_INC)/linux
  CFLAGS+=-z noexecstack
  LDFLAGS+=-lrt
endif
CFLAGS+=$(JAVA_INC) -I $(TOP_DIR)/include -Wno-strict-aliasing -Wno-deprecated-copy
MCL_LIB=$(TOP_DIR)/lib/libmcl.a

PACKAGE_NAME=com.herumi.mcl
PACKAGE_DIR=$(subst .,/,$(PACKAGE_NAME))

ELGAMAL_LIB=$(TOP_DIR)/lib/libmclelgamaljava.$(LIB_SUF)
MCLJAVA_LIB=$(TOP_DIR)/lib/libmcljava.$(LIB_SUF)
JAVA_EXE=cd $(TOP_DIR)/lib && LD_LIBRARY_PATH=../lib:$(LD_LIBRARY_PATH) java -classpath ../ffi/java
all: $(ELGAMAL_LIB)

elgamal_wrap.cxx: elgamal.i elgamal_impl.hpp
	$(MKDIR) $(PACKAGE_DIR)
	swig -java -package $(PACKAGE_NAME) -outdir $(PACKAGE_DIR) -c++ -Wall elgamal.i

mcl_wrap.cxx: mcl.i mcl_impl.hpp
	$(MKDIR) $(PACKAGE_DIR)
	swig -java -package $(PACKAGE_NAME) -outdir $(PACKAGE_DIR) -c++ -Wall mcl.i

$(MCL_LIB):
	make -C $(TOP_DIR) lib/libmcl.a

$(ELGAMAL_LIB): elgamal_wrap.cxx $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(CFLAGS) $(MCL_LIB) $(LDFLAGS) -shared

$(MCLJAVA_LIB): mcl_wrap.cxx $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(CFLAGS) $(MCL_LIB) $(LDFLAGS) -shared

%.class: %.java
	javac $<

ElgamalTest.class: ElgamalTest.java $(ELGAMAL_LIB)
MclTest.class: MclTest.java $(MCLJAVA_LIB)

jar:
	jar cvf mcl.jar com

test_elgamal: ElgamalTest.class $(ELGAMAL_LIB)
	$(JAVA_EXE) ElgamalTest
	$(JAVA_EXE) ElgamalTest -e NIST_P192
	$(JAVA_EXE) ElgamalTest -e NIST_P256 -h sha256
	$(JAVA_EXE) ElgamalTest -e secp256k1 -h sha256
	$(JAVA_EXE) ElgamalTest -e NIST_P384 -h sha384
#	$(JAVA_EXE) ElgamalTest -e NIST_P521 -h sha512

test_mcl: MclTest.class $(MCLJAVA_LIB)
	$(JAVA_EXE) MclTest

test:
	$(MAKE) test_elgamal
	$(MAKE) test_mcl

clean:
	rm -rf *.class $(ELGAMAL_LIB) $(MCLJAVA_LIB) $(PACKAGE_DIR)/*.class *_wrap.cxx

