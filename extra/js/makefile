
BOOST=.
EMCXX=em++
EMCXXFLAGS=-O3 -std=c++14 --bind \
	-I ../.. \
	-I $(BOOST)
WASM_BACKEND=0

ALL= \
	out/immer.asmjs.js \
	out/immer.asmjs.html \
	out/immer.wasm.js \
	out/immer.wasm.html

all: $(ALL)

out/immer.asmjs.js: immer.cpp
	@mkdir -p $(@D)
	$(EMCXX) $(EMCXXFLAGS) $< -o $@

out/immer.wasm.js: immer.cpp
	@mkdir -p $(@D)
	EMCC_WASM_BACKEND=$(WASM_BACKEND) $(EMCXX) -s WASM=1 $(EMCXXFLAGS) $< -o $@

%.html: %.js index.tpl.html
	sed s/%IMMER_JS%/`basename $<`/ < index.tpl.html > $@

clean:
	rm -f $(ALL)
