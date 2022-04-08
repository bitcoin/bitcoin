all: low_test

CFLAGS=-I ../include/ -Ofast -Wall -Wextra -DNDEBUG

low_test: low_test.cpp ../src/low_func_wasm.hpp
	$(CXX) -o low_test low_test.cpp $(CFLAGS) -m32

sqr.h: gen.py
	python3 $< > $@

sqr_test.exe: sqr_test.cpp sqr.h
	$(CXX) -o $@ $< $(CFLAGS) -lgmp -lgmpxx

sqr_test: sqr_test.exe
	./sqr_test.exe

clean:
	$(RM) sqr_test.exe sqr.h
