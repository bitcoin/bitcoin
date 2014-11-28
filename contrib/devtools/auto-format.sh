#!/bin/bash

if [ ! -f src/main.cpp ]; then
    echo "This tool must be run from the project root directory." >&2
    exit 1
fi

cd src

find . \( -name bip32_tests.cpp -o -name utilstrencodings.cpp -o -name chainparams.cpp -o -name chainparamsseed.h -o -name tinyformat.h -o -name qt -o -name leveldb -o -name univalue -o -name secp256k1 -o -name json \) -prune -o -name '*.h' -o -name '*.cpp' -print | while read FILE; do
    clang-format-3.5 -style=file -i "$FILE"
done

