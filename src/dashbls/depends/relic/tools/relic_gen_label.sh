#!/bin/bash

cat << PREAMBLE
/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Symbol renaming to a#undef clashes when simultaneous linking multiple builds.
 *
 * @ingroup core
 */

#ifndef RLC_LABEL_H
#define RLC_LABEL_H

#include "relic_conf.h"

#define RLC_PREFIX(F)			_RLC_PREFIX(LABEL, F)
#define _RLC_PREFIX(A, B)		__RLC_PREFIX(A, B)
#define __RLC_PREFIX(A, B)		A ## _ ## B

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

#ifdef LABEL

PREAMBLE

REDEF() {
	cat "relic_$1.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_0-9]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "relic_$1.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | sed 's/\*//' | sed 's/const //' | sed -r 's/[a-z,_,0-9]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tRLC_PREFIX\(\1\)/'
	echo
}

REDEF2() {
	cat "relic_$1.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_0-9]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "relic_$1.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep '(' | sed 's/\*//' | sed 's/const //' | sed -r 's/[a-z,_,0-9]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tRLC_PREFIX\(\1\)/'
	echo
}

REDEF_LOW() {
	cat "low/relic_$1_low.h" | grep "$1_" | grep -v define | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "low/relic_$1_low.h" | grep "$1_" | grep -v define | grep -v @version | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tRLC_PREFIX\(\1\)/'
	echo
}

REDEF2_LOW() {
	cat "low/relic_$1_low.h" | grep "$2_" | grep -v define | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#undef \1/'
	echo
	cat "low/relic_$1_low.h" | grep "$2_" | grep -v define | grep -v @version | grep -v typedef | grep -v '\\' | grep -v '\}' | grep -v '^ \*' | sed 's/const //' | sed 's/\*//' | sed -r 's/[a-z,_]+ ([a-z,_,0-9]+)\(.*/\#define \1 \tRLC_PREFIX\(\1\)/'
	echo
}

echo "#undef first_ctx"
echo "#define first_ctx     RLC_PREFIX(first_ctx)"
echo "#undef core_ctx"
echo "#define core_ctx      RLC_PREFIX(core_ctx)"
echo
REDEF core
REDEF arch
REDEF bench
REDEF err
REDEF rand
REDEF test
REDEF util

echo "#undef conf_print"
echo "#define conf_print    RLC_PREFIX(conf_print)"
echo

echo "#undef dv_t"
echo "#define dv_t          RLC_PREFIX(dv_t)"
echo
REDEF dv
REDEF_LOW dv

echo "#undef bn_st"
echo "#undef bn_t"
echo "#define bn_st     	RLC_PREFIX(bn_st)"
echo "#define bn_t      	RLC_PREFIX(bn_t)"
echo
REDEF bn
REDEF_LOW bn

echo "#undef fp_st"
echo "#undef fp_t"
echo "#define fp_st	        RLC_PREFIX(fp_st)"
echo "#define fp_t          RLC_PREFIX(fp_t)"
echo
REDEF fp
REDEF_LOW fp

echo "#undef fp_st"
echo "#undef fp_t"
echo "#define fp_st	        RLC_PREFIX(fp_st)"
echo "#define fp_t          RLC_PREFIX(fp_t)"
echo
REDEF fb
REDEF_LOW fb

echo "#undef ep_st"
echo "#undef ep_t"
echo "#define ep_st         RLC_PREFIX(ep_st)"
echo "#define ep_t          RLC_PREFIX(ep_t)"
echo
REDEF ep

echo "#undef ed_st"
echo "#undef ed_t"
echo "#define ed_st         RLC_PREFIX(ed_st)"
echo "#define ed_t          RLC_PREFIX(ed_t)"
echo
REDEF ed

echo "#undef eb_st"
echo "#undef eb_t"
echo "#define eb_st         RLC_PREFIX(eb_st)"
echo "#define eb_t          RLC_PREFIX(eb_t)"
echo
REDEF eb

echo "#undef ep2_st"
echo "#undef ep2_t"
echo "#define ep2_st        RLC_PREFIX(ep2_st)"
echo "#define ep2_t         RLC_PREFIX(ep2_t)"
echo
REDEF2 epx ep2

echo "#undef fp2_st"
echo "#undef fp2_t"
echo "#undef dv2_t"
echo "#define fp2_st        RLC_PREFIX(fp2_st)"
echo "#define fp2_t         RLC_PREFIX(fp2_t)"
echo "#define dv2_t         RLC_PREFIX(dv2_t)"
echo "#undef fp3_st"
echo "#undef fp3_t"
echo "#undef dv3_t"
echo "#define fp3_st        RLC_PREFIX(fp3_st)"
echo "#define fp3_t         RLC_PREFIX(fp3_t)"
echo "#define dv3_t         RLC_PREFIX(dv3_t)"
echo "#undef fp6_st"
echo "#undef fp6_t"
echo "#undef dv6_t"
echo "#define fp6_st        RLC_PREFIX(fp6_st)"
echo "#define fp6_t         RLC_PREFIX(fp6_t)"
echo "#define dv6_t         RLC_PREFIX(dv6_t)"
echo "#undef fp9_st"
echo "#undef fp8_t"
echo "#undef dv8_t"
echo "#define fp8_st        RLC_PREFIX(fp8_st)"
echo "#define fp8_t         RLC_PREFIX(fp8_t)"
echo "#define dv8_t         RLC_PREFIX(dv8_t)"
echo "#undef fp9_st"
echo "#undef fp9_t"
echo "#undef dv9_t"
echo "#define fp9_st        RLC_PREFIX(fp9_st)"
echo "#define fp9_t         RLC_PREFIX(fp9_t)"
echo "#define dv9_t         RLC_PREFIX(dv9_t)"
echo "#undef fp12_st"
echo "#undef fp12_t"
echo "#undef dv12_t"
echo "#define fp12_st        RLC_PREFIX(fp12_st)"
echo "#define fp12_t         RLC_PREFIX(fp12_t)"
echo "#define dv12_t         RLC_PREFIX(dv12_t)"
echo "#undef fp18_st"
echo "#undef fp18_t"
echo "#undef dv18_t"
echo "#define fp18_st        RLC_PREFIX(fp18_st)"
echo "#define fp18_t         RLC_PREFIX(fp18_t)"
echo "#define dv18_t         RLC_PREFIX(dv18_t)"
echo "#undef fp24_st"
echo "#undef fp24_t"
echo "#undef dv24_t"
echo "#define fp24_st        RLC_PREFIX(fp24_st)"
echo "#define fp24_t         RLC_PREFIX(fp24_t)"
echo "#define dv24_t         RLC_PREFIX(dv24_t)"
echo "#undef fp48_st"
echo "#undef fp48_t"
echo "#undef dv48_t"
echo "#define fp48_st        RLC_PREFIX(fp48_st)"
echo "#define fp48_t         RLC_PREFIX(fp48_t)"
echo "#define dv48_t         RLC_PREFIX(dv48_t)"
echo "#undef fp54_st"
echo "#undef fp54_t"
echo "#undef dv54_t"
echo "#define fp54_st        RLC_PREFIX(fp54_st)"
echo "#define fp54_t         RLC_PREFIX(fp54_t)"
echo "#define dv54_t         RLC_PREFIX(dv54_t)"
echo
REDEF2 fpx fp2
REDEF2_LOW fpx fp2
REDEF2 fpx fp3
REDEF2_LOW fpx fp3
REDEF2 fpx fp4
REDEF2 fpx fp6
REDEF2 fpx fp8
REDEF2 fpx fp9
REDEF2 fpx fp12
REDEF2 fpx fp18
REDEF2 fpx fp24
REDEF2 fpx fp48
REDEF2 fpx fp54

REDEF2 fbx fb2
REDEF2 fbx fb4

REDEF pp

echo "#undef crt_t"
echo "#undef rsa_t"
echo "#undef rabin_t"
echo "#undef phpe_t"
echo "#undef bdpe_t"
echo "#undef sokaka_t"
echo "#define crt_t		RLC_PREFIX(crt_t)"
echo "#define rsa_t		RLC_PREFIX(rsa_t)"
echo "#define rabin_t	RLC_PREFIX(rabin_t)"
echo "#define phpe_t	RLC_PREFIX(phpe_t)"
echo "#define bdpe_t	RLC_PREFIX(bdpe_t)"
echo "#define sokaka_t	RLC_PREFIX(sokaka_t)"
echo
REDEF cp

REDEF md

echo "#endif /* LABEL */"
echo
echo "#endif /* !RLC_LABEL_H */"
