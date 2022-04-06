TOP_DIR=../..
MCL_DIR?=$(TOP_DIR)/mcl
include $(MCL_DIR)/common.mk
ifeq ($(UNAME_S),Darwin)
  JAVA_INC_DIR?=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/JavaVM.framework/Headers/
  JAVA_INC?=-I$(JAVA_INC_DIR)
  CFLAGS+=$(JAVA_INC)/darwin
else
  JAVA_INC_DIR?=/usr/lib/jvm/default-java/include
  JAVA_INC?=-I$(JAVA_INC_DIR)
  CFLAGS+=$(JAVA_INC)/linux
  CFLAGS+=-z noexecstack
  LDFLAGS+=-lrt
endif
CFLAGS+=$(JAVA_INC) -I $(TOP_DIR)/include -I $(MCL_DIR)/include -Wno-strict-aliasing -Wno-deprecated-copy -Wno-cast-qual
ifeq ($(BLS_ETH),1)
  CFLAGS+=-DBLS_ETH
endif
MCL_LIB=$(TOP_DIR)/lib/libmcl.a

PACKAGE_NAME=com.herumi.bls
PACKAGE_DIR=$(subst .,/,$(PACKAGE_NAME))

BLSJAVA_LIB=$(TOP_DIR)/lib/libblsjava.$(LIB_SUF)
JAVA_EXE=cd $(TOP_DIR)/lib && LD_LIBRARY_PATH=../lib:$(LD_LIBRARY_PATH) java -classpath ../ffi/java
all: $(BLSJAVA_LIB)

bls_wrap.cxx: bls.i bls_impl.hpp
	$(MKDIR) $(PACKAGE_DIR)
	swig -java -package $(PACKAGE_NAME) -outdir $(PACKAGE_DIR) -c++ -Wall bls.i

$(MCL_LIB):
	make -C $(MCL_DIR) lib/libmcl.a

../../obj/bls_c384_256.o:
	make -C $(TOP_DIR) obj/bls_c384_256.o

$(BLSJAVA_LIB): bls_wrap.cxx bls_impl.hpp ../../obj/bls_c384_256.o #$(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(CFLAGS) $(LDFLAGS) -shared ../../obj/bls_c384_256.o $(MCL_DIR)/lib/libmcl.a

%.class: %.java
	javac $<

BlsTest.class: BlsTest.java

jar:
	jar cvf bls.jar com

test: BlsTest.class $(BLSJAVA_LIB)
	$(JAVA_EXE) BlsTest

clean:
	rm -rf *.class $(BLSJAVA_LIB) $(PACKAGE_DIR)/*.class *_wrap.cxx $(TOP_DIR)/obj/*.o

