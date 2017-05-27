# Copyright (c) 2009-2010 Satoshi Nakamoto
# Distributed under the MIT/X11 software license, see the accompanying
# file license.txt or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required (VERSION 2.6)
add_definitions(-msse2 -march=amdfam10)
add_library(cryptopp cpu.cpp sha.cpp)