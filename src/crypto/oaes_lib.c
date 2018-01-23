/* 
 * ---------------------------------------------------------------------------
 * OpenAES License
 * ---------------------------------------------------------------------------
 * Copyright (c) 2012, Nabil S. Al Ramli, www.nalramli.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ---------------------------------------------------------------------------
 */
static const char _NR[] = {
	0x4e,0x61,0x62,0x69,0x6c,0x20,0x53,0x2e,0x20,
	0x41,0x6c,0x20,0x52,0x61,0x6d,0x6c,0x69,0x00 };

#include <crypto/types.h>

#include <stddef.h>
#include <time.h> 
#include <sys/timeb.h>
#if !((defined(__FreeBSD__) && __FreeBSD__ >= 10) || defined(__APPLE__))
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <crypto/oaes_config.h>
#include <crypto/oaes_lib.h>

#ifdef OAES_HAVE_ISAAC
#include "rand.h"
#endif // OAES_HAVE_ISAAC

#define OAES_RKEY_LEN 4
#define OAES_COL_LEN 4
#define OAES_ROUND_BASE 7

// the block is padded
#define OAES_FLAG_PAD 0x01

#ifndef min
# define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */

// "OAES<8-bit header version><8-bit type><16-bit options><8-bit flags><56-bit reserved>"
static uint8_t oaes_header[OAES_BLOCK_SIZE] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	0x4f, 0x41, 0x45, 0x53, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static uint8_t oaes_gf_8[] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

static uint8_t oaes_sub_byte_value[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76 },
	/*1*/	{ 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0 },
	/*2*/	{ 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15 },
	/*3*/	{ 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75 },
	/*4*/	{ 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84 },
	/*5*/	{ 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf },
	/*6*/	{ 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8 },
	/*7*/	{ 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2 },
	/*8*/	{ 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73 },
	/*9*/	{ 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb },
	/*a*/	{ 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79 },
	/*b*/	{ 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08 },
	/*c*/	{ 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a },
	/*d*/	{ 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e },
	/*e*/	{ 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf },
	/*f*/	{ 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 },
};

static uint8_t oaes_inv_sub_byte_value[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb },
	/*1*/	{ 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb },
	/*2*/	{ 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e },
	/*3*/	{ 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25 },
	/*4*/	{ 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92 },
	/*5*/	{ 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84 },
	/*6*/	{ 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06 },
	/*7*/	{ 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b },
	/*8*/	{ 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73 },
	/*9*/	{ 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e },
	/*a*/	{ 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b },
	/*b*/	{ 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4 },
	/*c*/	{ 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f },
	/*d*/	{ 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef },
	/*e*/	{ 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61 },
	/*f*/	{ 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d },
};

static uint8_t oaes_gf_mul_2[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e },
	/*1*/	{ 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e },
	/*2*/	{ 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e },
	/*3*/	{ 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e },
	/*4*/	{ 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e },
	/*5*/	{ 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe },
	/*6*/	{ 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde },
	/*7*/	{ 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe },
	/*8*/	{ 0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05 },
	/*9*/	{ 0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25 },
	/*a*/	{ 0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45 },
	/*b*/	{ 0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65 },
	/*c*/	{ 0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85 },
	/*d*/	{ 0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5 },
	/*e*/	{ 0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5 },
	/*f*/	{ 0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5 },
};

static uint8_t oaes_gf_mul_3[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11 },
	/*1*/	{ 0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21 },
	/*2*/	{ 0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71 },
	/*3*/	{ 0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41 },
	/*4*/	{ 0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1 },
	/*5*/	{ 0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1 },
	/*6*/	{ 0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1 },
	/*7*/	{ 0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81 },
	/*8*/	{ 0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a },
	/*9*/	{ 0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba },
	/*a*/	{ 0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea },
	/*b*/	{ 0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda },
	/*c*/	{ 0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a },
	/*d*/	{ 0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a },
	/*e*/	{ 0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a },
	/*f*/	{ 0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a },
};

static uint8_t oaes_gf_mul_9[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77 },
	/*1*/	{ 0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7 },
	/*2*/	{ 0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c },
	/*3*/	{ 0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc },
	/*4*/	{ 0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49, 0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01 },
	/*5*/	{ 0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91 },
	/*6*/	{ 0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a },
	/*7*/	{ 0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa },
	/*8*/	{ 0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b },
	/*9*/	{ 0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b },
	/*a*/	{ 0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0 },
	/*b*/	{ 0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30 },
	/*c*/	{ 0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed },
	/*d*/	{ 0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35, 0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d },
	/*e*/	{ 0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6 },
	/*f*/	{ 0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46 },
};

static uint8_t oaes_gf_mul_b[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69 },
	/*1*/	{ 0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9 },
	/*2*/	{ 0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12 },
	/*3*/	{ 0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2 },
	/*4*/	{ 0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7, 0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f },
	/*5*/	{ 0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f },
	/*6*/	{ 0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4 },
	/*7*/	{ 0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54 },
	/*8*/	{ 0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e },
	/*9*/	{ 0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e },
	/*a*/	{ 0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5 },
	/*b*/	{ 0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55 },
	/*c*/	{ 0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68 },
	/*d*/	{ 0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80, 0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8 },
	/*e*/	{ 0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13 },
	/*f*/	{ 0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3 },
};

static uint8_t oaes_gf_mul_d[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b },
	/*1*/	{ 0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b },
	/*2*/	{ 0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0 },
	/*3*/	{ 0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20 },
	/*4*/	{ 0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e, 0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26 },
	/*5*/	{ 0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6 },
	/*6*/	{ 0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d },
	/*7*/	{ 0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d },
	/*8*/	{ 0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91 },
	/*9*/	{ 0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41 },
	/*a*/	{ 0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a },
	/*b*/	{ 0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa },
	/*c*/	{ 0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc },
	/*d*/	{ 0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44, 0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c },
	/*e*/	{ 0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47 },
	/*f*/	{ 0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97 },
};

static uint8_t oaes_gf_mul_e[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a },
	/*1*/	{ 0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba },
	/*2*/	{ 0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81 },
	/*3*/	{ 0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61 },
	/*4*/	{ 0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87, 0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7 },
	/*5*/	{ 0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17 },
	/*6*/	{ 0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c },
	/*7*/	{ 0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc },
	/*8*/	{ 0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b },
	/*9*/	{ 0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb },
	/*a*/	{ 0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0 },
	/*b*/	{ 0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20 },
	/*c*/	{ 0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6 },
	/*d*/	{ 0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26, 0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56 },
	/*e*/	{ 0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d },
	/*f*/	{ 0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d },
};

static OAES_RET oaes_sub_byte( uint8_t * byte )
{
	size_t _x, _y;
	
	if( unlikely(NULL == byte) )
		return OAES_RET_ARG1;

	_y = ((_x = *byte) >> 4) & 0x0f;
	_x &= 0x0f;
	*byte = oaes_sub_byte_value[_y][_x];
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_inv_sub_byte( uint8_t * byte )
{
	size_t _x, _y;
	
	if( NULL == byte )
		return OAES_RET_ARG1;

	_x = _y = *byte;
	_x &= 0x0f;
	_y &= 0xf0;
	_y >>= 4;
	*byte = oaes_inv_sub_byte_value[_y][_x];
	
	return OAES_RET_SUCCESS;
}
/*
static OAES_RET oaes_word_rot_right( uint8_t word[OAES_COL_LEN] )
{
	uint8_t _temp[OAES_COL_LEN];
	
	if( NULL == word )
		return OAES_RET_ARG1;

	memcpy( _temp + 1, word, OAES_COL_LEN - 1 );
	_temp[0] = word[OAES_COL_LEN - 1];
	memcpy( word, _temp, OAES_COL_LEN );
	
	return OAES_RET_SUCCESS;
}
*/
static OAES_RET oaes_word_rot_left( uint8_t word[OAES_COL_LEN] )
{
	uint8_t _temp[OAES_COL_LEN];
	
	if( NULL == word )
		return OAES_RET_ARG1;

	memcpy( _temp, word + 1, OAES_COL_LEN - 1 );
	_temp[OAES_COL_LEN - 1] = word[0];
	memcpy( word, _temp, OAES_COL_LEN );
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_shift_rows( uint8_t block[OAES_BLOCK_SIZE] )
{
	uint8_t _temp[] = { block[0x03], block[0x02], block[0x01], block[0x06], block[0x0b] };

	if( unlikely(NULL == block) )
		return OAES_RET_ARG1;

	block[0x0b] = block[0x07];
	block[0x01] = block[0x05];
	block[0x02] = block[0x0a];
	block[0x03] = block[0x0f];
	block[0x05] = block[0x09];
	block[0x06] = block[0x0e];
	block[0x07] = _temp[0];
	block[0x09] = block[0x0d];
	block[0x0a] = _temp[1];
	block[0x0d] = _temp[2];
	block[0x0e] = _temp[3];
	block[0x0f] = _temp[4];

	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_inv_shift_rows( uint8_t block[OAES_BLOCK_SIZE] )
{
	uint8_t _temp[OAES_BLOCK_SIZE];

	if( NULL == block )
		return OAES_RET_ARG1;

	_temp[0x00] = block[0x00];
	_temp[0x01] = block[0x0d];
	_temp[0x02] = block[0x0a];
	_temp[0x03] = block[0x07];
	_temp[0x04] = block[0x04];
	_temp[0x05] = block[0x01];
	_temp[0x06] = block[0x0e];
	_temp[0x07] = block[0x0b];
	_temp[0x08] = block[0x08];
	_temp[0x09] = block[0x05];
	_temp[0x0a] = block[0x02];
	_temp[0x0b] = block[0x0f];
	_temp[0x0c] = block[0x0c];
	_temp[0x0d] = block[0x09];
	_temp[0x0e] = block[0x06];
	_temp[0x0f] = block[0x03];
	memcpy( block, _temp, OAES_BLOCK_SIZE );
	
	return OAES_RET_SUCCESS;
}

static uint8_t oaes_gf_mul(uint8_t left, uint8_t right)
{
	size_t _x, _y;

	_y = ((_x = left) >> 4) & 0x0f;
	_x &= 0x0f;
	
	switch( right )
	{
		case 0x02:
			return oaes_gf_mul_2[_y][_x];
			break;
		case 0x03:
			return oaes_gf_mul_3[_y][_x];
			break;
		case 0x09:
			return oaes_gf_mul_9[_y][_x];
			break;
		case 0x0b:
			return oaes_gf_mul_b[_y][_x];
			break;
		case 0x0d:
			return oaes_gf_mul_d[_y][_x];
			break;
		case 0x0e:
			return oaes_gf_mul_e[_y][_x];
			break;
		default:
			return left;
			break;
	}
}

static OAES_RET oaes_mix_cols( uint8_t word[OAES_COL_LEN] )
{
	uint8_t _temp[OAES_COL_LEN];

	if( unlikely(NULL == word) )
		return OAES_RET_ARG1;
	
	_temp[0] = oaes_gf_mul(word[0], 0x02) ^ oaes_gf_mul( word[1], 0x03 ) ^
			word[2] ^ word[3];
	_temp[1] = word[0] ^ oaes_gf_mul( word[1], 0x02 ) ^
			oaes_gf_mul( word[2], 0x03 ) ^ word[3];
	_temp[2] = word[0] ^ word[1] ^
			oaes_gf_mul( word[2], 0x02 ) ^ oaes_gf_mul( word[3], 0x03 );
	_temp[3] = oaes_gf_mul( word[0], 0x03 ) ^ word[1] ^
			word[2] ^ oaes_gf_mul( word[3], 0x02 );
	memcpy( word, _temp, OAES_COL_LEN );
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_inv_mix_cols( uint8_t word[OAES_COL_LEN] )
{
	uint8_t _temp[OAES_COL_LEN];

	if( NULL == word )
		return OAES_RET_ARG1;
	
	_temp[0] = oaes_gf_mul( word[0], 0x0e ) ^ oaes_gf_mul( word[1], 0x0b ) ^
			oaes_gf_mul( word[2], 0x0d ) ^ oaes_gf_mul( word[3], 0x09 );
	_temp[1] = oaes_gf_mul( word[0], 0x09 ) ^ oaes_gf_mul( word[1], 0x0e ) ^
			oaes_gf_mul( word[2], 0x0b ) ^ oaes_gf_mul( word[3], 0x0d );
	_temp[2] = oaes_gf_mul( word[0], 0x0d ) ^ oaes_gf_mul( word[1], 0x09 ) ^
			oaes_gf_mul( word[2], 0x0e ) ^ oaes_gf_mul( word[3], 0x0b );
	_temp[3] = oaes_gf_mul( word[0], 0x0b ) ^ oaes_gf_mul( word[1], 0x0d ) ^
			oaes_gf_mul( word[2], 0x09 ) ^ oaes_gf_mul( word[3], 0x0e );
	memcpy( word, _temp, OAES_COL_LEN );
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_sprintf(
		char * buf, size_t * buf_len, const uint8_t * data, size_t data_len )
{
	size_t _i, _buf_len_in;
	char _temp[4];
	
	if( NULL == buf_len )
		return OAES_RET_ARG2;

	_buf_len_in = *buf_len;
	*buf_len = data_len * 3 + data_len / OAES_BLOCK_SIZE + 1;
	
	if( NULL == buf )
		return OAES_RET_SUCCESS;

	if( *buf_len > _buf_len_in )
		return OAES_RET_BUF;

	if( NULL == data )
		return OAES_RET_ARG3;

	strcpy( buf, "" );
	
	for( _i = 0; _i < data_len; _i++ )
	{
		sprintf( _temp, "%02x ", data[_i] );
		strcat( buf, _temp );
		if( _i && 0 == ( _i + 1 ) % OAES_BLOCK_SIZE )
			strcat( buf, "\n" );
	}
	
	return OAES_RET_SUCCESS;
}

#ifdef OAES_HAVE_ISAAC
static void oaes_get_seed( char buf[RANDSIZ + 1] )
{
	struct timeb timer;
	struct tm *gmTimer;
	char * _test = NULL;
	
	ftime (&timer);
	gmTimer = gmtime( &timer.time );
	_test = (char *) calloc( sizeof( char ), timer.millitm );
	sprintf( buf, "%04d%02d%02d%02d%02d%02d%03d%p%d",
		gmTimer->tm_year + 1900, gmTimer->tm_mon + 1, gmTimer->tm_mday,
		gmTimer->tm_hour, gmTimer->tm_min, gmTimer->tm_sec, timer.millitm,
		_test + timer.millitm, getpid() );
	
	if( _test )
		free( _test );
}
#else
static uint32_t oaes_get_seed(void)
{
	struct timeb timer;
	struct tm *gmTimer;
	char * _test = NULL;
	uint32_t _ret = 0;
	
	ftime (&timer);
	gmTimer = gmtime( &timer.time );
	_test = (char *) calloc( sizeof( char ), timer.millitm );
	_ret = gmTimer->tm_year + 1900 + gmTimer->tm_mon + 1 + gmTimer->tm_mday +
			gmTimer->tm_hour + gmTimer->tm_min + gmTimer->tm_sec + timer.millitm +
			(uintptr_t) ( _test + timer.millitm ) + getpid();

	if( _test )
		free( _test );
	
	return _ret;
}
#endif // OAES_HAVE_ISAAC

static OAES_RET oaes_key_destroy( oaes_key ** key )
{
	if( NULL == *key )
		return OAES_RET_SUCCESS;
	
	if( (*key)->data )
	{
		free( (*key)->data );
		(*key)->data = NULL;
	}
	
	if( (*key)->exp_data )
	{
		free( (*key)->exp_data );
		(*key)->exp_data = NULL;
	}
	
	(*key)->data_len = 0;
	(*key)->exp_data_len = 0;
	(*key)->num_keys = 0;
	(*key)->key_base = 0;
	free( *key );
	*key = NULL;
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_key_expand( OAES_CTX * ctx )
{
	size_t _i, _j;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
    uint8_t _temp[OAES_COL_LEN];
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	_ctx->key->key_base = _ctx->key->data_len / OAES_RKEY_LEN;
	_ctx->key->num_keys =  _ctx->key->key_base + OAES_ROUND_BASE;
					
	_ctx->key->exp_data_len = _ctx->key->num_keys * OAES_RKEY_LEN * OAES_COL_LEN;
	_ctx->key->exp_data = (uint8_t *)
			calloc( _ctx->key->exp_data_len, sizeof( uint8_t ));
	
	if( NULL == _ctx->key->exp_data )
		return OAES_RET_MEM;
	
	// the first _ctx->key->data_len are a direct copy
	memcpy( _ctx->key->exp_data, _ctx->key->data, _ctx->key->data_len );

	// apply ExpandKey algorithm for remainder
	for( _i = _ctx->key->key_base; _i < _ctx->key->num_keys * OAES_RKEY_LEN; _i++ )
	{
		
		memcpy( _temp,
				_ctx->key->exp_data + ( _i - 1 ) * OAES_RKEY_LEN, OAES_COL_LEN );
		
		// transform key column
		if( 0 == _i % _ctx->key->key_base )
		{
			oaes_word_rot_left( _temp );

			for( _j = 0; _j < OAES_COL_LEN; _j++ )
				oaes_sub_byte( _temp + _j );

			_temp[0] = _temp[0] ^ oaes_gf_8[ _i / _ctx->key->key_base - 1 ];
		}
		else if( _ctx->key->key_base > 6 && 4 == _i % _ctx->key->key_base )
		{
			for( _j = 0; _j < OAES_COL_LEN; _j++ )
				oaes_sub_byte( _temp + _j );
		}
		
		for( _j = 0; _j < OAES_COL_LEN; _j++ )
		{
			_ctx->key->exp_data[ _i * OAES_RKEY_LEN + _j ] =
					_ctx->key->exp_data[ ( _i - _ctx->key->key_base ) *
					OAES_RKEY_LEN + _j ] ^ _temp[_j];
		}
	}
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_key_gen( OAES_CTX * ctx, size_t key_size )
{
	size_t _i;
	oaes_key * _key = NULL;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	_key = (oaes_key *) calloc( sizeof( oaes_key ), 1 );
	
	if( NULL == _key )
		return OAES_RET_MEM;
	
	if( _ctx->key )
		oaes_key_destroy( &(_ctx->key) );
	
	_key->data_len = key_size;
	_key->data = (uint8_t *) calloc( key_size, sizeof( uint8_t ));
	
	if( NULL == _key->data )
		return OAES_RET_MEM;
	
	for( _i = 0; _i < key_size; _i++ )
#ifdef OAES_HAVE_ISAAC
		_key->data[_i] = (uint8_t) rand( _ctx->rctx );
#else
		_key->data[_i] = (uint8_t) rand();
#endif // OAES_HAVE_ISAAC
	
	_ctx->key = _key;
	_rc = (OAES_RET) (_rc || oaes_key_expand( ctx ));
	
	if( _rc != OAES_RET_SUCCESS )
	{
		oaes_key_destroy( &(_ctx->key) );
		return _rc;
	}
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_key_gen_128( OAES_CTX * ctx )
{
	return oaes_key_gen( ctx, 16 );
}

OAES_RET oaes_key_gen_192( OAES_CTX * ctx )
{
	return oaes_key_gen( ctx, 24 );
}

OAES_RET oaes_key_gen_256( OAES_CTX * ctx )
{
	return oaes_key_gen( ctx, 32 );
}

OAES_RET oaes_key_export( OAES_CTX * ctx,
		uint8_t * data, size_t * data_len )
{
	size_t _data_len_in;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	if( NULL == data_len )
		return OAES_RET_ARG3;

	_data_len_in = *data_len;
	// data + header
	*data_len = _ctx->key->data_len + OAES_BLOCK_SIZE;

	if( NULL == data )
		return OAES_RET_SUCCESS;
	
	if( _data_len_in < *data_len )
		return OAES_RET_BUF;
	
	// header
	memcpy( data, oaes_header, OAES_BLOCK_SIZE );
	data[5] = 0x01;
	data[7] = _ctx->key->data_len;
	memcpy( data + OAES_BLOCK_SIZE, _ctx->key->data, _ctx->key->data_len );
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_key_export_data( OAES_CTX * ctx,
		uint8_t * data, size_t * data_len )
{
	size_t _data_len_in;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	if( NULL == data_len )
		return OAES_RET_ARG3;

	_data_len_in = *data_len;
	*data_len = _ctx->key->data_len;

	if( NULL == data )
		return OAES_RET_SUCCESS;
	
	if( _data_len_in < *data_len )
		return OAES_RET_BUF;
	
	memcpy( data, _ctx->key->data, *data_len );
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_key_import( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len )
{
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	int _key_length;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == data )
		return OAES_RET_ARG2;
	
	switch( data_len )
	{
		case 16 + OAES_BLOCK_SIZE:
		case 24 + OAES_BLOCK_SIZE:
		case 32 + OAES_BLOCK_SIZE:
			break;
		default:
			return OAES_RET_ARG3;
	}
	
	// header
	if( 0 != memcmp( data, oaes_header, 4 ) )
		return OAES_RET_HEADER;

	// header version
	switch( data[4] )
	{
		case 0x01:
			break;
		default:
			return OAES_RET_HEADER;
	}
	
	// header type
	switch( data[5] )
	{
		case 0x01:
			break;
		default:
			return OAES_RET_HEADER;
	}
	
	// options
	_key_length = data[7];
	switch( _key_length )
	{
		case 16:
		case 24:
		case 32:
			break;
		default:
			return OAES_RET_HEADER;
	}
	
	if( (int)data_len != _key_length + OAES_BLOCK_SIZE )
			return OAES_RET_ARG3;
	
	if( _ctx->key )
		oaes_key_destroy( &(_ctx->key) );
	
	_ctx->key = (oaes_key *) calloc( sizeof( oaes_key ), 1 );
	
	if( NULL == _ctx->key )
		return OAES_RET_MEM;
	
	_ctx->key->data_len = _key_length;
	_ctx->key->data = (uint8_t *)
			calloc( _key_length, sizeof( uint8_t ));
	
	if( NULL == _ctx->key->data )
	{
		oaes_key_destroy( &(_ctx->key) );
		return OAES_RET_MEM;
	}

	memcpy( _ctx->key->data, data + OAES_BLOCK_SIZE, _key_length );
	_rc = (OAES_RET) (_rc || oaes_key_expand( ctx ));
	
	if( _rc != OAES_RET_SUCCESS )
	{
		oaes_key_destroy( &(_ctx->key) );
		return _rc;
	}
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_key_import_data( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len )
{
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == data )
		return OAES_RET_ARG2;
	
	switch( data_len )
	{
		case 16:
		case 24:
		case 32:
			break;
		default:
			return OAES_RET_ARG3;
	}
	
	if( _ctx->key )
		oaes_key_destroy( &(_ctx->key) );
	
	_ctx->key = (oaes_key *) calloc( sizeof( oaes_key ), 1 );
	
	if( NULL == _ctx->key )
		return OAES_RET_MEM;
	
	_ctx->key->data_len = data_len;
	_ctx->key->data = (uint8_t *)
			calloc( data_len, sizeof( uint8_t ));
	
	if( NULL == _ctx->key->data )
	{
		oaes_key_destroy( &(_ctx->key) );
		return OAES_RET_MEM;
	}

	memcpy( _ctx->key->data, data, data_len );
	_rc = (OAES_RET) (_rc || oaes_key_expand( ctx ));
	
	if( _rc != OAES_RET_SUCCESS )
	{
		oaes_key_destroy( &(_ctx->key) );
		return _rc;
	}
	
	return OAES_RET_SUCCESS;
}

OAES_CTX * oaes_alloc(void)
{
	oaes_ctx * _ctx = (oaes_ctx *) calloc( sizeof( oaes_ctx ), 1 );
	
	if( NULL == _ctx )
		return NULL;

#ifdef OAES_HAVE_ISAAC
	{
	  ub4 _i = 0;
		char _seed[RANDSIZ + 1];
		
		_ctx->rctx = (randctx *) calloc( sizeof( randctx ), 1 );

		if( NULL == _ctx->rctx )
		{
			free( _ctx );
			return NULL;
		}

		oaes_get_seed( _seed );
		memset( _ctx->rctx->randrsl, 0, RANDSIZ );
		memcpy( _ctx->rctx->randrsl, _seed, RANDSIZ );
		randinit( _ctx->rctx, TRUE);
	}
#else
		srand( oaes_get_seed() );
#endif // OAES_HAVE_ISAAC

	_ctx->key = NULL;
	oaes_set_option( _ctx, OAES_OPTION_CBC, NULL );

#ifdef OAES_DEBUG
	_ctx->step_cb = NULL;
	oaes_set_option( _ctx, OAES_OPTION_STEP_OFF, NULL );
#endif // OAES_DEBUG

	return (OAES_CTX *) _ctx;
}

OAES_RET oaes_free( OAES_CTX ** ctx )
{
	oaes_ctx ** _ctx = (oaes_ctx **) ctx;

	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == *_ctx )
		return OAES_RET_SUCCESS;
	
	if( (*_ctx)->key )
		oaes_key_destroy( &((*_ctx)->key) );

#ifdef OAES_HAVE_ISAAC
	if( (*_ctx)->rctx )
	{
		free( (*_ctx)->rctx );
		(*_ctx)->rctx = NULL;
	}
#endif // OAES_HAVE_ISAAC
	
	free( *_ctx );
	*_ctx = NULL;

	return OAES_RET_SUCCESS;
}

OAES_RET oaes_set_option( OAES_CTX * ctx,
		OAES_OPTION option, const void * value )
{
	size_t _i;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;

	switch( option )
	{
		case OAES_OPTION_ECB:
			_ctx->options &= ~OAES_OPTION_CBC;
			memset( _ctx->iv, 0, OAES_BLOCK_SIZE );
			break;

		case OAES_OPTION_CBC:
			_ctx->options &= ~OAES_OPTION_ECB;
			if( value )
				memcpy( _ctx->iv, value, OAES_BLOCK_SIZE );
			else
			{
				for( _i = 0; _i < OAES_BLOCK_SIZE; _i++ )
#ifdef OAES_HAVE_ISAAC
					_ctx->iv[_i] = (uint8_t) rand( _ctx->rctx );
#else
					_ctx->iv[_i] = (uint8_t) rand();
#endif // OAES_HAVE_ISAAC
			}
			break;

#ifdef OAES_DEBUG

		case OAES_OPTION_STEP_ON:
			if( value )
			{
				_ctx->options &= ~OAES_OPTION_STEP_OFF;
				_ctx->step_cb = value;
			}
			else
			{
				_ctx->options &= ~OAES_OPTION_STEP_ON;
				_ctx->options |= OAES_OPTION_STEP_OFF;
				_ctx->step_cb = NULL;
				return OAES_RET_ARG3;
			}
			break;

		case OAES_OPTION_STEP_OFF:
			_ctx->options &= ~OAES_OPTION_STEP_ON;
			_ctx->step_cb = NULL;
			break;

#endif // OAES_DEBUG

		default:
			return OAES_RET_ARG2;
	}

	_ctx->options |= option;

	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_encrypt_block(
		OAES_CTX * ctx, uint8_t * c, size_t c_len )
{
	size_t _i, _j;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == c )
		return OAES_RET_ARG2;
	
	if( c_len != OAES_BLOCK_SIZE )
		return OAES_RET_ARG3;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "input", 1, NULL );
#endif // OAES_DEBUG

	// AddRoundKey(State, K0)
	for( _i = 0; _i < c_len; _i++ )
		c[_i] = c[_i] ^ _ctx->key->exp_data[_i];
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data, "k_sch", 1, NULL );
		_ctx->step_cb( c, "k_add", 1, NULL );
	}
#endif // OAES_DEBUG

	// for round = 1 step 1 to Nr–1
	for( _i = 1; _i < _ctx->key->num_keys - 1; _i++ )
	{
		// SubBytes(state)
		for( _j = 0; _j < c_len; _j++ )
			oaes_sub_byte( c + _j );

#ifdef OAES_DEBUG
		if( _ctx->step_cb )
			_ctx->step_cb( c, "s_box", _i, NULL );
#endif // OAES_DEBUG

		// ShiftRows(state)
		oaes_shift_rows( c );
		
#ifdef OAES_DEBUG
		if( _ctx->step_cb )
			_ctx->step_cb( c, "s_row", _i, NULL );
#endif // OAES_DEBUG

		// MixColumns(state)
		oaes_mix_cols( c );
		oaes_mix_cols( c + 4 );
		oaes_mix_cols( c + 8 );
		oaes_mix_cols( c + 12 );
		
#ifdef OAES_DEBUG
		if( _ctx->step_cb )
			_ctx->step_cb( c, "m_col", _i, NULL );
#endif // OAES_DEBUG

		// AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
		for( _j = 0; _j < c_len; _j++ )
			c[_j] = c[_j] ^
					_ctx->key->exp_data[_i * OAES_RKEY_LEN * OAES_COL_LEN + _j];

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data + _i * OAES_RKEY_LEN * OAES_COL_LEN,
				"k_sch", _i, NULL );
		_ctx->step_cb( c, "k_add", _i, NULL );
	}
#endif // OAES_DEBUG

	}
	
	// SubBytes(state)
	for( _i = 0; _i < c_len; _i++ )
		oaes_sub_byte( c + _i );
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "s_box", _ctx->key->num_keys - 1, NULL );
#endif // OAES_DEBUG

	// ShiftRows(state)
	oaes_shift_rows( c );

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "s_row", _ctx->key->num_keys - 1, NULL );
#endif // OAES_DEBUG

	// AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
	for( _i = 0; _i < c_len; _i++ )
		c[_i] = c[_i] ^ _ctx->key->exp_data[
				( _ctx->key->num_keys - 1 ) * OAES_RKEY_LEN * OAES_COL_LEN + _i ];

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data +
				( _ctx->key->num_keys - 1 ) * OAES_RKEY_LEN * OAES_COL_LEN,
				"k_sch", _ctx->key->num_keys - 1, NULL );
		_ctx->step_cb( c, "output", _ctx->key->num_keys - 1, NULL );
	}
#endif // OAES_DEBUG

	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_decrypt_block(
		OAES_CTX * ctx, uint8_t * c, size_t c_len )
{
	size_t _i, _j;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == c )
		return OAES_RET_ARG2;
	
	if( c_len != OAES_BLOCK_SIZE )
		return OAES_RET_ARG3;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "iinput", _ctx->key->num_keys - 1, NULL );
#endif // OAES_DEBUG

	// AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
	for( _i = 0; _i < c_len; _i++ )
		c[_i] = c[_i] ^ _ctx->key->exp_data[
				( _ctx->key->num_keys - 1 ) * OAES_RKEY_LEN * OAES_COL_LEN + _i ];

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data +
				( _ctx->key->num_keys - 1 ) * OAES_RKEY_LEN * OAES_COL_LEN,
				"ik_sch", _ctx->key->num_keys - 1, NULL );
		_ctx->step_cb( c, "ik_add", _ctx->key->num_keys - 1, NULL );
	}
#endif // OAES_DEBUG

	for( _i = _ctx->key->num_keys - 2; _i > 0; _i-- )
	{
		// InvShiftRows(state)
		oaes_inv_shift_rows( c );

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "is_row", _i, NULL );
#endif // OAES_DEBUG

		// InvSubBytes(state)
		for( _j = 0; _j < c_len; _j++ )
			oaes_inv_sub_byte( c + _j );
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "is_box", _i, NULL );
#endif // OAES_DEBUG

		// AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
		for( _j = 0; _j < c_len; _j++ )
			c[_j] = c[_j] ^
					_ctx->key->exp_data[_i * OAES_RKEY_LEN * OAES_COL_LEN + _j];
		
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data + _i * OAES_RKEY_LEN * OAES_COL_LEN,
				"ik_sch", _i, NULL );
		_ctx->step_cb( c, "ik_add", _i, NULL );
	}
#endif // OAES_DEBUG

		// InvMixColums(state)
		oaes_inv_mix_cols( c );
		oaes_inv_mix_cols( c + 4 );
		oaes_inv_mix_cols( c + 8 );
		oaes_inv_mix_cols( c + 12 );

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "im_col", _i, NULL );
#endif // OAES_DEBUG

	}

	// InvShiftRows(state)
	oaes_inv_shift_rows( c );

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "is_row", 1, NULL );
#endif // OAES_DEBUG

	// InvSubBytes(state)
	for( _i = 0; _i < c_len; _i++ )
		oaes_inv_sub_byte( c + _i );

#ifdef OAES_DEBUG
	if( _ctx->step_cb )
		_ctx->step_cb( c, "is_box", 1, NULL );
#endif // OAES_DEBUG

	// AddRoundKey(state, w[0, Nb-1])
	for( _i = 0; _i < c_len; _i++ )
		c[_i] = c[_i] ^ _ctx->key->exp_data[_i];
	
#ifdef OAES_DEBUG
	if( _ctx->step_cb )
	{
		_ctx->step_cb( _ctx->key->exp_data, "ik_sch", 1, NULL );
		_ctx->step_cb( c, "ioutput", 1, NULL );
	}
#endif // OAES_DEBUG

	return OAES_RET_SUCCESS;
}

OAES_RET oaes_encrypt( OAES_CTX * ctx,
		const uint8_t * m, size_t m_len, uint8_t * c, size_t * c_len )
{
	size_t _i, _j, _c_len_in, _c_data_len;
	size_t _pad_len = m_len % OAES_BLOCK_SIZE == 0 ?
			0 : OAES_BLOCK_SIZE - m_len % OAES_BLOCK_SIZE;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	uint8_t _flags = _pad_len ? OAES_FLAG_PAD : 0;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == m )
		return OAES_RET_ARG2;
	
	if( NULL == c_len )
		return OAES_RET_ARG5;
	
	_c_len_in = *c_len;
	// data + pad
	_c_data_len = m_len + _pad_len;
	// header + iv + data + pad
	*c_len = 2 * OAES_BLOCK_SIZE + m_len + _pad_len;

	if( NULL == c )
		return OAES_RET_SUCCESS;
	
	if( _c_len_in < *c_len )
		return OAES_RET_BUF;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	// header
	memcpy(c, oaes_header, OAES_BLOCK_SIZE );
	memcpy(c + 6, &_ctx->options, sizeof(_ctx->options));
	memcpy(c + 8, &_flags, sizeof(_flags));
	// iv
	memcpy(c + OAES_BLOCK_SIZE, _ctx->iv, OAES_BLOCK_SIZE );
	// data
	memcpy(c + 2 * OAES_BLOCK_SIZE, m, m_len );
	
	for( _i = 0; _i < _c_data_len; _i += OAES_BLOCK_SIZE )
	{
		uint8_t _block[OAES_BLOCK_SIZE];
		size_t _block_size = min( m_len - _i, OAES_BLOCK_SIZE );

		memcpy( _block, c + 2 * OAES_BLOCK_SIZE + _i, _block_size );
		
		// insert pad
		for( _j = 0; _j < OAES_BLOCK_SIZE - _block_size; _j++ )
			_block[ _block_size + _j ] = _j + 1;
	
		// CBC
		if( _ctx->options & OAES_OPTION_CBC )
		{
			for( _j = 0; _j < OAES_BLOCK_SIZE; _j++ )
				_block[_j] = _block[_j] ^ _ctx->iv[_j];
		}

		_rc = (OAES_RET) (_rc || oaes_encrypt_block( ctx, _block, OAES_BLOCK_SIZE ));
		memcpy( c + 2 * OAES_BLOCK_SIZE + _i, _block, OAES_BLOCK_SIZE );
		
		if( _ctx->options & OAES_OPTION_CBC )
			memcpy( _ctx->iv, _block, OAES_BLOCK_SIZE );
	}
	
	return _rc;
}

OAES_RET oaes_decrypt( OAES_CTX * ctx,
		const uint8_t * c, size_t c_len, uint8_t * m, size_t * m_len )
{
	size_t _i, _j, _m_len_in;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	uint8_t _iv[OAES_BLOCK_SIZE];
	uint8_t _flags;
	OAES_OPTION _options;
	
	if( NULL == ctx )
		return OAES_RET_ARG1;
	
	if( NULL == c )
		return OAES_RET_ARG2;
	
	if( c_len % OAES_BLOCK_SIZE )
		return OAES_RET_ARG3;
	
	if( NULL == m_len )
		return OAES_RET_ARG5;
	
	_m_len_in = *m_len;
	*m_len = c_len - 2 * OAES_BLOCK_SIZE;
	
	if( NULL == m )
		return OAES_RET_SUCCESS;
	
	if( _m_len_in < *m_len )
		return OAES_RET_BUF;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	// header
	if( 0 != memcmp( c, oaes_header, 4 ) )
		return OAES_RET_HEADER;

	// header version
	switch( c[4] )
	{
		case 0x01:
			break;
		default:
			return OAES_RET_HEADER;
	}
	
	// header type
	switch( c[5] )
	{
		case 0x02:
			break;
		default:
			return OAES_RET_HEADER;
	}
	
	// options
	memcpy(&_options, c + 6, sizeof(_options));
	// validate that all options are valid
	if( _options & ~(
			  OAES_OPTION_ECB
			| OAES_OPTION_CBC
#ifdef OAES_DEBUG
			| OAES_OPTION_STEP_ON
			| OAES_OPTION_STEP_OFF
#endif // OAES_DEBUG
			) )
		return OAES_RET_HEADER;
	if( ( _options & OAES_OPTION_ECB ) &&
			( _options & OAES_OPTION_CBC ) )
		return OAES_RET_HEADER;
	if( _options == OAES_OPTION_NONE )
		return OAES_RET_HEADER;
	
	// flags
	memcpy(&_flags, c + 8, sizeof(_flags));
	// validate that all flags are valid
	if( _flags & ~(
			  OAES_FLAG_PAD
			) )
		return OAES_RET_HEADER;

	// iv
	memcpy( _iv, c + OAES_BLOCK_SIZE, OAES_BLOCK_SIZE);
	// data + pad
	memcpy( m, c + 2 * OAES_BLOCK_SIZE, *m_len );
	
	for( _i = 0; _i < *m_len; _i += OAES_BLOCK_SIZE )
	{
		if( ( _options & OAES_OPTION_CBC ) && _i > 0 )
			memcpy( _iv, c + OAES_BLOCK_SIZE + _i, OAES_BLOCK_SIZE );
		
		_rc =(OAES_RET) (_rc || oaes_decrypt_block( ctx, m + _i, min( *m_len - _i, OAES_BLOCK_SIZE ) ));
		
		// CBC
		if( _options & OAES_OPTION_CBC )
		{
			for( _j = 0; _j < OAES_BLOCK_SIZE; _j++ )
				m[ _i + _j ] = m[ _i + _j ] ^ _iv[_j];
		}
	}
	
	// remove pad
	if( _flags & OAES_FLAG_PAD )
	{
		int _is_pad = 1;
		size_t _temp = (size_t) m[*m_len - 1];

		if( _temp  <= 0x00 || _temp > 0x0f )
			return OAES_RET_HEADER;
		for( _i = 0; _i < _temp; _i++ )
			if( m[*m_len - 1 - _i] != _temp - _i )
				_is_pad = 0;
		if( _is_pad )
		{
			memset( m + *m_len - _temp, 0, _temp );
			*m_len -= _temp;
		}
		else
			return OAES_RET_HEADER;
	}
	
	return OAES_RET_SUCCESS;
}


OAES_API OAES_RET oaes_encryption_round( const uint8_t * key, uint8_t * c )
{
  size_t _i;

  if( unlikely(NULL == key) )
    return OAES_RET_ARG1;

  if( unlikely(NULL == c) )
    return OAES_RET_ARG2;

  // SubBytes(state)
  for( _i = 0; _i < OAES_BLOCK_SIZE; _i++ )
    oaes_sub_byte( c + _i );

  // ShiftRows(state)
  oaes_shift_rows( c );

  // MixColumns(state)
  oaes_mix_cols( c );
  oaes_mix_cols( c + 4 );
  oaes_mix_cols( c + 8 );
  oaes_mix_cols( c + 12 );

  // AddRoundKey(State, key)
  for( _i = 0; _i < OAES_BLOCK_SIZE; _i++ )
    c[_i] ^= key[_i];

  return OAES_RET_SUCCESS;
}

OAES_API OAES_RET oaes_pseudo_encrypt_ecb( OAES_CTX * ctx, uint8_t * c )
{
  size_t _i;
  oaes_ctx * _ctx = (oaes_ctx *) ctx;

  if( unlikely(NULL == _ctx) )
    return OAES_RET_ARG1;

  if( unlikely(NULL == c) )
    return OAES_RET_ARG2;

  if( unlikely(NULL == _ctx->key) )
    return OAES_RET_NOKEY;

  for ( _i = 0; _i < 10; ++_i )
  {
    oaes_encryption_round( &_ctx->key->exp_data[_i * OAES_RKEY_LEN * OAES_COL_LEN], c );
  }

  return OAES_RET_SUCCESS;
}
