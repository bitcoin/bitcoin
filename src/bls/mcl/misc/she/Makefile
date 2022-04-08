include ../../common.mk

ALL_SRC=she_min.cpp bench.cpp

TARGET=$(ALL_SRC:.cpp=.exe)
all: $(TARGET)

CFLAGS+=-I../../include -std=c++11
ifeq ($(OS),mac)
  CFLAGS+=-Xpreprocessor -fopenmp
  LDFLAGS+=-lomp
else
  CFLAGS+=-fopenmp
  LDFLAGS+=-fopenmp
endif
ifneq ($(MCL_MAX_BIT_SIZE),)
  CFLAGS+=-DMCL_MAX_BIT_SIZE=$(MCL_MAX_BIT_SIZE)
endif

MCL_LIB=../../lib/libmcl.a

$(MCL_LIB):
	$(MAKE) -C ../../ lib/libmcl.a

%.o: %.cpp
	$(PRE)$(CXX) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.exe: %.o $(MCL_LIB)
	$(PRE)$(CXX) $< -o $@ $(MCL_LIB) $(LDFLAGS)

clean:
	rm -rf $(TARGET) *.o edit_test *.d

DEPEND_FILE=$(ALL_SRC:.cpp=.d)
-include $(DEPEND_FILE)

# don't remove these files automatically
.SECONDARY: $(ALL_SRC:.cpp=.o)

