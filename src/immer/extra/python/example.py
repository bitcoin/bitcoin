#!/usr/bin/env python##

# immer: immutable data structures for C++
# Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
#
# This software is distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt


# include:intro/start
import immer

v0 = immer.Vector().append(13).append(42)
assert v0[0] == 13
assert v0[1] == 42
assert len(v0) == 2

v1 = v0.set(0, 12)
assert v0.tolist() == [13, 42]
assert v1.tolist() == [12, 42]
# include:intro/end
