#!/bin/sh
hex=$(base58 -d 25 -c 19DXstMaV43WpYg4ceREiiTv2UntmoiA9j | xxd -p)
test x$hex != x005a1fc5dd9e6f03819fca94a2d89669469667f9a1
