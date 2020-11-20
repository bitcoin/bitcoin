all:
	c++ -O3 -Wall -shared -std=c++11 -undefined dynamic_lookup `python3 -m pybind11 --includes` -I../../../src ../../../src/crypto/chacha20.cpp chacha20_bindings.cpp -o chacha20_bindings`python3-config --extension-suffix`

clean:
	rm -f *`python3-config --extension-suffix`
