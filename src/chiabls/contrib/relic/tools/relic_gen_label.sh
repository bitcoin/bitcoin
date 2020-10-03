#!/bin/bash

cat << PREAMBLE
/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Symbol renaming to a#undef clashes when simultaneous linking multiple builds.
 *
 * @ingroup core
 */

#ifndef RELIC_LABEL_H
#define RELIC_LABEL_H

#include "relic_conf.h"

#define PREFIX(F)		_PREFIX(LABEL, F)
#define _PREFIX(A, B)		__PREFIX(A, B)
#define __PREFIX(A, B)		A ## _ ## B

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

#ifdef LABEL

PREAMBLE

REDEF() {
	cat "relic_$1.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_0-9]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "relic_$1.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | sed 's/\*//' | sed 's/const //' | sed -r 's/[a-z,_,0-9]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tPREFIX\(\1\)/'
	echo
}

REDEF2() {
	cat "relic_$1.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_0-9]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "relic_$1.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | sed 's/\*//' | sed 's/const //' | sed -r 's/[a-z,_,0-9]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tPREFIX\(\1\)/'
	echo
}

REDEF_LOW() {
	cat "low/relic_$1_low.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "low/relic_$1_low.h" | grep "$1_" | grep -v define | grep -v @version | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tPREFIX\(\1\)/'
	echo
}

REDEF2_LOW() {
	cat "low/relic_$1_low.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "low/relic_$1_low.h" | grep "$2_" | grep -v define | grep -v @version | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tPREFIX\(\1\)/'
	echo
}

echo "#undef first_ctx"
echo "#define first_ctx	PREFIX(first_ctx)"
echo "#undef core_ctx"
echo "#define core_ctx	PREFIX(core_ctx)"
echo
REDEF core
REDEF arch
REDEF bench
REDEF err
REDEF rand
REDEF pool
REDEF test
REDEF trace
REDEF util

echo "#undef conf_print"
echo "#define conf_print       PREFIX(conf_print)"
echo

echo "#undef dv_t"
echo "#define dv_t	PREFIX(dv_t)"
echo
REDEF dv
REDEF_LOW dv

echo "#undef bn_st"
echo "#undef bn_t"
echo "#define bn_st	PREFIX(bn_st)"
echo "#define bn_t	PREFIX(bn_t)"
echo
REDEF bn
REDEF_LOW bn

echo "#undef fp_st"
echo "#undef fp_t"
echo "#define fp_st	PREFIX(fp_st)"
echo "#define fp_t	PREFIX(fp_t)"
echo
REDEF fp
REDEF_LOW fp

echo "#undef fp_st"
echo "#undef fp_t"
echo "#define fp_st	PREFIX(fp_st)"
echo "#define fp_t	PREFIX(fp_t)"
echo
REDEF fb
REDEF_LOW fb

echo "#undef ep_st"
echo "#undef ep_t"
echo "#define ep_st	PREFIX(ep_st)"
echo "#define ep_t	PREFIX(ep_t)"
echo
REDEF ep

echo "#undef ed_st"
echo "#undef ed_t"
echo "#define ed_st	PREFIX(ed_st)"
echo "#define ed_t	PREFIX(ed_t)"
echo
REDEF ed

echo "#undef eb_st"
echo "#undef eb_t"
echo "#define eb_st	PREFIX(eb_st)"
echo "#define eb_t	PREFIX(eb_t)"
echo
REDEF eb

echo "#undef ep2_st"
echo "#undef ep2_t"
echo "#define ep2_st	PREFIX(ep2_st)"
echo "#define ep2_t		PREFIX(ep2_t)"
echo
REDEF2 epx ep2

echo "#undef fp2_st"
echo "#undef fp2_t"
echo "#undef dv2_t"
echo "#undef fp3_st"
echo "#undef fp3_t"
echo "#undef dv3_t"
echo "#undef fp6_st"
echo "#undef fp6_t"
echo "#undef dv6_t"
echo "#undef fp12_t"
echo "#undef fp18_t"
echo
REDEF2 fpx fp2
REDEF2_LOW fpx fp2
REDEF2 fpx fp3
REDEF2_LOW fpx fp3
REDEF2 fpx fp6
REDEF2 fpx fp12
REDEF2 fpx fp18

REDEF2 fbx fb2
REDEF2 fbx fb4

REDEF pp

echo "#undef rsa_t"
echo "#undef rabin_t"
echo "#undef bdpe_t"
echo "#undef sokaka_t"
echo "#define rsa_t		PREFIX(rsa_t)"
echo "#define rabin_t	PREFIX(rabin_t)"
echo "#define bdpe_t	PREFIX(bdpe_t)"
echo "#define sokaka_t	PREFIX(sokaka_t)"
echo
REDEF cp

echo "#endif /* LABEL */"
echo
echo "#endif /* !RELIC_LABEL_H */"
