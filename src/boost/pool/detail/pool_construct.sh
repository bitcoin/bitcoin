#!/bin/sh
#
# Copyright (C) 2000 Stephen Cleary
#
# Distributed under the Boost Software License, Version 1.0. (See accompany-
# ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# See http://www.boost.org for updates, documentation, and revision history.
#

m4 -P -E -DNumberOfArguments=$1 pool_construct.m4 > pool_construct.ipp

