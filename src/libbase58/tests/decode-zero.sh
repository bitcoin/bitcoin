#!/bin/sh
hex=$(base58 -d 25 111111 | xxd -p)
test x$hex = x000000000000
