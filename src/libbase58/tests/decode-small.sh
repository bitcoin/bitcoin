#!/bin/sh
hex=$(base58 -d 4 2 | xxd -p)
test x$hex = x01
