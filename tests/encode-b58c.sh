#!/bin/sh
b58=$(echo '005a1fc5dd9e6f03819fca94a2d89669469667f9a0' | xxd -r -p | base58 -c)
test x$b58 = x19DXstMaV43WpYg4ceREiiTv2UntmoiA9j
