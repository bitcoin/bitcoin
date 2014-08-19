#!/bin/sh
hex=$(base58 -d 25 -c 19DXstMaV43WpYg4ceREiiTv2UntmoiA9a | xxd -p)
test x$hex = x
