
# immer: immutable data structures for C++
# Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
#
# This software is distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt

##

import immer
import pyrsistent

BENCHMARK_SIZE = 1000

def push(v, n=BENCHMARK_SIZE):
    for x in xrange(n):
        v = v.append(x)
    return v

def assoc(v):
    for i in xrange(len(v)):
        v = v.set(i, i+1)
    return v

def index(v):
    for i in xrange(len(v)):
        v[i]

def test_push_immer(benchmark):
    benchmark(push, immer.Vector())

def test_push_pyrsistent(benchmark):
    benchmark(push, pyrsistent.pvector())

def test_assoc_immer(benchmark):
    benchmark(assoc, push(immer.Vector()))

def test_assoc_pyrsistent(benchmark):
    benchmark(assoc, push(pyrsistent.pvector()))

def test_index_immer(benchmark):
    benchmark(index, push(immer.Vector()))

def test_index_pyrsistent(benchmark):
    benchmark(index, push(pyrsistent.pvector()))
