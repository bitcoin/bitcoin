/* Generic 64-bit integer implementation of Curve25519 ECDH
 * Written by Matthijs van Duin, 200608242056
 * Pared down by Scott W. Dunlop, 20080808
 *
 * Public domain.
 *
 * Based on work by Daniel J Bernstein, http://cr.yp.to/ecdh.html
 */

#include <stdint.h>
#include <string.h>

extern "C" {

#include "curve25519_i64.h"

typedef int32_t i25519[10];
typedef const int32_t *i25519ptr;
typedef const uint8_t *srcptr;
typedef uint8_t *dstptr;

typedef struct expstep expstep;

struct expstep {
	unsigned nsqr;
	unsigned muli;
};


/********************* constants *********************/

const k25519
zero25519 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
prime25519 = { 237, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 127 },
order25519 = { 237, 211, 245, 92, 26, 99, 18, 88, 214, 156, 247, 162, 222, 249,
		222, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16 };

/* smallest multiple of the order that's >= 2^255 */
static const k25519
order_times_8 = { 104, 159, 174, 231, 210, 24, 147, 192, 178, 230, 188, 23,
	245, 206, 247, 166, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128 };

/* constants 2Gy and 1/(2Gy) */
static const i25519
base_2y = { 39999547, 18689728, 59995525, 1648697, 57546132,
		24010086, 19059592, 5425144, 63499247, 16420658 },
base_r2y = { 5744, 8160848, 4790893, 13779497, 35730846,
		12541209, 49101323, 30047407, 40071253, 6226132 };


/********************* radix 2^8 math *********************/

static void cpy32(k25519 d, const k25519 s) {
	int i;
	for (i = 0; i < 32; i++) d[i] = s[i];
}

/* p[m..n+m-1] = q[m..n+m-1] + z * x */
/* n is the size of x */
/* n+m is the size of p and q */
static inline
int mula_small(dstptr p, srcptr q, unsigned m, srcptr x, unsigned n, int z) {
	int v = 0;
	unsigned i;
	for (i = 0; i < n; i++) {
		p[i+m] = v += q[i+m] + z * x[i];
		v >>= 8;
	}
	return v;
}

/* p += x * y * z  where z is a small integer
 * x is size 32, y is size t, p is size 32+t
 * y is allowed to overlap with p+32 if you don't care about the upper half  */
static int mula32(dstptr p, srcptr x, srcptr y, unsigned t, int z) {
	const unsigned n = 31;
	int w = 0;
	unsigned i;
	for (i = 0; i < t; i++) {
		int zy = z * y[i];
		p[i+n] = w += mula_small(p, p, i, x, n, zy) + p[i+n] + zy * x[n];
		w >>= 8;
	}
	p[i+n] += w;
	return w >> 8;
}

/* divide r (size n) by d (size t), returning quotient q and remainder r
 * quotient is size n-t+1, remainder is size t
 * requires t > 0 && d[t-1] != 0
 * requires that r[-1] and d[-1] are valid memory locations
 * q may overlap with r+t */
static void divmod(dstptr q, dstptr r, unsigned n, srcptr d, unsigned t) {
	int rn = 0;
	int dt = d[t-1] << 8;
	if (t > 1)
		dt |= d[t-2];

	while (n-- >= t) {
		int z = (rn << 16) | (r[n] << 8);
		if (n > 0)
			z |= r[n-1];
		z /= dt;
		rn += mula_small(r,r, n-t+1, d, t, -z);
		q[n-t+1] = z + rn; /* rn is 0 or -1 (underflow) */
		mula_small(r,r, n-t+1, d, t, -rn);
		rn = r[n];
		r[n] = 0;
	}

	r[t-1] = rn;
}

static inline unsigned numsize(srcptr x, unsigned n) {
	while (n-- && !x[n])
		;
	return n+1;
}

/* Returns x if a contains the gcd, y if b.  Also, the returned buffer contains 
 * the inverse of a mod b, as 32-byte signed.
 * x and y must have 64 bytes space for temporary use.
 * requires that a[-1] and b[-1] are valid memory locations  */
static dstptr egcd32(dstptr x, dstptr y, dstptr a, dstptr b) {
	unsigned an, bn = 32, qn, i;
	for (i = 0; i < 32; i++)
		x[i] = y[i] = 0;
	x[0] = 1;
	if (!(an = numsize(a, 32)))
		return y;	/* division by zero */
	while (42) {
		qn = bn - an + 1;
		divmod(y+32, b, bn, a, an);
		if (!(bn = numsize(b, bn)))
			return x;
		mula32(y, x, y+32, qn, -1);

		qn = an - bn + 1;
		divmod(x+32, a, an, b, bn);
		if (!(an = numsize(a, an)))
			return y;
		mula32(x, y, x+32, qn, -1);
	}
}


/********************* radix 2^25.5 GF(2^255-19) math *********************/

#define P25 33554431	/* (1 << 25) - 1 */
#define P26 67108863	/* (1 << 26) - 1 */


/* debugging code */

#ifdef WASP_DEBUG_CURVE
#include <stdio.h>
#include <stdlib.h>
static void check_range(const char *where, int32_t x, int32_t lb, int32_t ub) {
	if (x < lb || x > ub) {
		fprintf(stderr, "%s check failed: %08x (%d)\n", where, x, x);
		abort();
	}
}
static void check_nonred(const char *where, const i25519 x) {
	int i;
	for (i = 0; i < 10; i++)
		check_range(where, x[i], -185861411, 185861411);
}
static void check_reduced(const char *where, const i25519 x) {
	int i;
	for (i = 0; i < 9; i++)
		check_range(where, x[i], 0, P26 >> (i & 1));
	check_range(where, x[9], -675, P25+675);
}
#else
#define check_range(w, x, l, u)
#define check_nonred(w, x)
#define check_reduced(w, x)
#endif

/* convenience macros */

#define M(i) ((uint32_t) m[i])
#define X(i) ((int64_t) x[i])
#define m64(arg1,arg2) ((int64_t) (arg1) * (arg2))

/* Convert to internal format from little-endian byte format */
static void unpack25519(i25519 x, const k25519 m) {
	x[0] =  M( 0)         | M( 1)<<8 | M( 2)<<16 | (M( 3)& 3)<<24;
	x[1] = (M( 3)&~ 3)>>2 | M( 4)<<6 | M( 5)<<14 | (M( 6)& 7)<<22;
	x[2] = (M( 6)&~ 7)>>3 | M( 7)<<5 | M( 8)<<13 | (M( 9)&31)<<21;
	x[3] = (M( 9)&~31)>>5 | M(10)<<3 | M(11)<<11 | (M(12)&63)<<19;
	x[4] = (M(12)&~63)>>6 | M(13)<<2 | M(14)<<10 |  M(15)    <<18;
	x[5] =  M(16)         | M(17)<<8 | M(18)<<16 | (M(19)& 1)<<24;
	x[6] = (M(19)&~ 1)>>1 | M(20)<<7 | M(21)<<15 | (M(22)& 7)<<23;
	x[7] = (M(22)&~ 7)>>3 | M(23)<<5 | M(24)<<13 | (M(25)&15)<<21;
	x[8] = (M(25)&~15)>>4 | M(26)<<4 | M(27)<<12 | (M(28)&63)<<20;
	x[9] = (M(28)&~63)>>6 | M(29)<<2 | M(30)<<10 |  M(31)    <<18;
	check_reduced("unpack output", x);
}


/* Check if reduced-form input >= 2^255-19 */
static inline int is_overflow(const i25519 x) {
	return ((x[0] > P26-19) & ((x[1] & x[3] & x[5] & x[7] & x[9]) == P25) &
	                          ((x[2] & x[4] & x[6] & x[8]) == P26)
	       ) | (x[9] > P25);
}


/* Convert from internal format to little-endian byte format.  The 
 * number must be in a reduced form which is output by the following ops:
 *     unpack, mul, sqr
 *     set --  if input in range 0 .. P25
 * If you're unsure if the number is reduced, first multiply it by 1.  */
static void pack25519(const i25519 x, k25519 m) {
	int32_t ld = 0, ud = 0;
	int64_t t;
	check_reduced("pack input", x);
	ld = is_overflow(x) - (x[9] < 0);
	ud = ld * -(P25+1);
	ld *= 19;
	t = ld + X(0) + (X(1) << 26);
	m[ 0] = t; m[ 1] = t >> 8; m[ 2] = t >> 16; m[ 3] = t >> 24;
	t = (t >> 32) + (X(2) << 19);
	m[ 4] = t; m[ 5] = t >> 8; m[ 6] = t >> 16; m[ 7] = t >> 24;
	t = (t >> 32) + (X(3) << 13);
	m[ 8] = t; m[ 9] = t >> 8; m[10] = t >> 16; m[11] = t >> 24;
	t = (t >> 32) + (X(4) <<  6);
	m[12] = t; m[13] = t >> 8; m[14] = t >> 16; m[15] = t >> 24;
	t = (t >> 32) + X(5) + (X(6) << 25);
	m[16] = t; m[17] = t >> 8; m[18] = t >> 16; m[19] = t >> 24;
	t = (t >> 32) + (X(7) << 19);
	m[20] = t; m[21] = t >> 8; m[22] = t >> 16; m[23] = t >> 24;
	t = (t >> 32) + (X(8) << 12);
	m[24] = t; m[25] = t >> 8; m[26] = t >> 16; m[27] = t >> 24;
	t = (t >> 32) + ((X(9) + ud) << 6);
	m[28] = t; m[29] = t >> 8; m[30] = t >> 16; m[31] = t >> 24;
}

/* Copy a number */
static void cpy25519(i25519 out, const i25519 in) {
	int i;
	for (i = 0; i < 10; i++)
		out[i] = in[i];
}

/* Set a number to value, which must be in range -185861411 .. 185861411 */
static void set25519(i25519 out, const int32_t in) {
	int i;
	out[0] = in;
	for (i = 1; i < 10; i++)
		out[i] = 0;
}

/* Add/subtract two numbers.  The inputs must be in reduced form, and the 
 * output isn't, so to do another addition or subtraction on the output, 
 * first multiply it by one to reduce it. */
static void add25519(i25519 xy, const i25519 x, const i25519 y) {
	xy[0] = x[0] + y[0];	xy[1] = x[1] + y[1];
	xy[2] = x[2] + y[2];	xy[3] = x[3] + y[3];
	xy[4] = x[4] + y[4];	xy[5] = x[5] + y[5];
	xy[6] = x[6] + y[6];	xy[7] = x[7] + y[7];
	xy[8] = x[8] + y[8];	xy[9] = x[9] + y[9];
}
static void sub25519(i25519 xy, const i25519 x, const i25519 y) {
	xy[0] = x[0] - y[0];	xy[1] = x[1] - y[1];
	xy[2] = x[2] - y[2];	xy[3] = x[3] - y[3];
	xy[4] = x[4] - y[4];	xy[5] = x[5] - y[5];
	xy[6] = x[6] - y[6];	xy[7] = x[7] - y[7];
	xy[8] = x[8] - y[8];	xy[9] = x[9] - y[9];
}

/* Multiply a number by a small integer in range -185861411 .. 185861411.
 * The output is in reduced form, the input x need not be.  x and xy may point
 * to the same buffer. */
static i25519ptr mul25519small(i25519 xy, const i25519 x, const int32_t y) {
	register int64_t t;
	check_nonred("mul small x input", x);
	check_range("mul small y input", y, -185861411, 185861411);
	t = m64(x[8],y);
	xy[8] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[9],y);
	xy[9] = t & ((1 << 25) - 1);
	t = 19 * (t >> 25) + m64(x[0],y);
	xy[0] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[1],y);
	xy[1] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[2],y);
	xy[2] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[3],y);
	xy[3] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[4],y);
	xy[4] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[5],y);
	xy[5] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[6],y);
	xy[6] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[7],y);
	xy[7] = t & ((1 << 25) - 1);
	t = (t >> 25) + xy[8];
	xy[8] = t & ((1 << 26) - 1);
	xy[9] += (int32_t)(t >> 26);
	check_reduced("mul small output", xy);
	return xy;
}

/* Multiply two numbers.  The output is in reduced form, the inputs need not 
 * be. */
static i25519ptr mul25519(i25519 xy, const i25519 x, const i25519 y) {
	register int64_t t;
	check_nonred("mul input x", x);
	check_nonred("mul input y", y);
	t = m64(x[0],y[8]) + m64(x[2],y[6]) + m64(x[4],y[4]) + m64(x[6],y[2]) +
		m64(x[8],y[0]) + 2 * (m64(x[1],y[7]) + m64(x[3],y[5]) +
				m64(x[5],y[3]) + m64(x[7],y[1])) + 38 *
		m64(x[9],y[9]);
	xy[8] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[0],y[9]) + m64(x[1],y[8]) + m64(x[2],y[7]) +
		m64(x[3],y[6]) + m64(x[4],y[5]) + m64(x[5],y[4]) +
		m64(x[6],y[3]) + m64(x[7],y[2]) + m64(x[8],y[1]) +
		m64(x[9],y[0]);
	xy[9] = t & ((1 << 25) - 1);
	t = m64(x[0],y[0]) + 19 * ((t >> 25) + m64(x[2],y[8]) + m64(x[4],y[6])
			+ m64(x[6],y[4]) + m64(x[8],y[2])) + 38 *
		(m64(x[1],y[9]) + m64(x[3],y[7]) + m64(x[5],y[5]) +
		 m64(x[7],y[3]) + m64(x[9],y[1]));
	xy[0] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[0],y[1]) + m64(x[1],y[0]) + 19 * (m64(x[2],y[9])
			+ m64(x[3],y[8]) + m64(x[4],y[7]) + m64(x[5],y[6]) +
			m64(x[6],y[5]) + m64(x[7],y[4]) + m64(x[8],y[3]) +
			m64(x[9],y[2]));
	xy[1] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[0],y[2]) + m64(x[2],y[0]) + 19 * (m64(x[4],y[8])
			+ m64(x[6],y[6]) + m64(x[8],y[4])) + 2 * m64(x[1],y[1])
			+ 38 * (m64(x[3],y[9]) + m64(x[5],y[7]) +
					m64(x[7],y[5]) + m64(x[9],y[3]));
	xy[2] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[0],y[3]) + m64(x[1],y[2]) + m64(x[2],y[1]) +
		m64(x[3],y[0]) + 19 * (m64(x[4],y[9]) + m64(x[5],y[8]) +
				m64(x[6],y[7]) + m64(x[7],y[6]) +
				m64(x[8],y[5]) + m64(x[9],y[4]));
	xy[3] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[0],y[4]) + m64(x[2],y[2]) + m64(x[4],y[0]) + 19 *
		(m64(x[6],y[8]) + m64(x[8],y[6])) + 2 * (m64(x[1],y[3]) +
							 m64(x[3],y[1])) + 38 *
		(m64(x[5],y[9]) + m64(x[7],y[7]) + m64(x[9],y[5]));
	xy[4] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[0],y[5]) + m64(x[1],y[4]) + m64(x[2],y[3]) +
		m64(x[3],y[2]) + m64(x[4],y[1]) + m64(x[5],y[0]) + 19 *
		(m64(x[6],y[9]) + m64(x[7],y[8]) + m64(x[8],y[7]) +
		 m64(x[9],y[6]));
	xy[5] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[0],y[6]) + m64(x[2],y[4]) + m64(x[4],y[2]) +
		m64(x[6],y[0]) + 19 * m64(x[8],y[8]) + 2 * (m64(x[1],y[5]) +
				m64(x[3],y[3]) + m64(x[5],y[1])) + 38 *
		(m64(x[7],y[9]) + m64(x[9],y[7]));
	xy[6] = t & ((1 << 26) - 1);
	t = (t >> 26) + m64(x[0],y[7]) + m64(x[1],y[6]) + m64(x[2],y[5]) +
		m64(x[3],y[4]) + m64(x[4],y[3]) + m64(x[5],y[2]) +
		m64(x[6],y[1]) + m64(x[7],y[0]) + 19 * (m64(x[8],y[9]) +
				m64(x[9],y[8]));
	xy[7] = t & ((1 << 25) - 1);
	t = (t >> 25) + xy[8];
	xy[8] = t & ((1 << 26) - 1);
	xy[9] += (int32_t)(t >> 26);
	check_reduced("mul output", xy);
	return xy;
}

/* Square a number.  Optimization of  mul25519(x2, x, x)  */
static i25519ptr sqr25519(i25519 x2, const i25519 x) {
	register int64_t t;
	check_nonred("sqr input", x);
	t = m64(x[4],x[4]) + 2 * (m64(x[0],x[8]) + m64(x[2],x[6])) + 38 *
		m64(x[9],x[9]) + 4 * (m64(x[1],x[7]) + m64(x[3],x[5]));
	x2[8] = t & ((1 << 26) - 1);
	t = (t >> 26) + 2 * (m64(x[0],x[9]) + m64(x[1],x[8]) + m64(x[2],x[7]) +
			m64(x[3],x[6]) + m64(x[4],x[5]));
	x2[9] = t & ((1 << 25) - 1);
	t = 19 * (t >> 25) + m64(x[0],x[0]) + 38 * (m64(x[2],x[8]) +
			m64(x[4],x[6]) + m64(x[5],x[5])) + 76 * (m64(x[1],x[9])
			+ m64(x[3],x[7]));
	x2[0] = t & ((1 << 26) - 1);
	t = (t >> 26) + 2 * m64(x[0],x[1]) + 38 * (m64(x[2],x[9]) +
			m64(x[3],x[8]) + m64(x[4],x[7]) + m64(x[5],x[6]));
	x2[1] = t & ((1 << 25) - 1);
	t = (t >> 25) + 19 * m64(x[6],x[6]) + 2 * (m64(x[0],x[2]) +
			m64(x[1],x[1])) + 38 * m64(x[4],x[8]) + 76 *
			(m64(x[3],x[9]) + m64(x[5],x[7]));
	x2[2] = t & ((1 << 26) - 1);
	t = (t >> 26) + 2 * (m64(x[0],x[3]) + m64(x[1],x[2])) + 38 *
		(m64(x[4],x[9]) + m64(x[5],x[8]) + m64(x[6],x[7]));
	x2[3] = t & ((1 << 25) - 1);
	t = (t >> 25) + m64(x[2],x[2]) + 2 * m64(x[0],x[4]) + 38 *
		(m64(x[6],x[8]) + m64(x[7],x[7])) + 4 * m64(x[1],x[3]) + 76 *
		m64(x[5],x[9]);
	x2[4] = t & ((1 << 26) - 1);
	t = (t >> 26) + 2 * (m64(x[0],x[5]) + m64(x[1],x[4]) + m64(x[2],x[3]))
		+ 38 * (m64(x[6],x[9]) + m64(x[7],x[8]));
	x2[5] = t & ((1 << 25) - 1);
	t = (t >> 25) + 19 * m64(x[8],x[8]) + 2 * (m64(x[0],x[6]) +
			m64(x[2],x[4]) + m64(x[3],x[3])) + 4 * m64(x[1],x[5]) +
			76 * m64(x[7],x[9]);
	x2[6] = t & ((1 << 26) - 1);
	t = (t >> 26) + 2 * (m64(x[0],x[7]) + m64(x[1],x[6]) + m64(x[2],x[5]) +
			m64(x[3],x[4])) + 38 * m64(x[8],x[9]);
	x2[7] = t & ((1 << 25) - 1);
	t = (t >> 25) + x2[8];
	x2[8] = t & ((1 << 26) - 1);
	x2[9] += (t >> 26);
	check_reduced("sqr output", x2);
	return x2;
}

/* Calculates a reciprocal.  The output is in reduced form, the inputs need not 
 * be.  Simply calculates  y = x^(p-2)  so it's not too fast. */
/* When sqrtassist is true, it instead calculates y = x^((p-5)/8) */
static void recip25519(i25519 y, const i25519 x, int sqrtassist) {
	i25519 t0, t1, t2, t3, t4;
	int i;
	/* the chain for x^(2^255-21) is straight from djb's implementation */
	sqr25519(t1, x);	/*  2 == 2 * 1	*/
	sqr25519(t2, t1);	/*  4 == 2 * 2	*/
	sqr25519(t0, t2);	/*  8 == 2 * 4	*/
	mul25519(t2, t0, x);	/*  9 == 8 + 1	*/
	mul25519(t0, t2, t1);	/* 11 == 9 + 2	*/
	sqr25519(t1, t0);	/* 22 == 2 * 11	*/
	mul25519(t3, t1, t2);	/* 31 == 22 + 9
				== 2^5   - 2^0	*/
	sqr25519(t1, t3);	/* 2^6   - 2^1	*/
	sqr25519(t2, t1);	/* 2^7   - 2^2	*/
	sqr25519(t1, t2);	/* 2^8   - 2^3	*/
	sqr25519(t2, t1);	/* 2^9   - 2^4	*/
	sqr25519(t1, t2);	/* 2^10  - 2^5	*/
	mul25519(t2, t1, t3);	/* 2^10  - 2^0	*/
	sqr25519(t1, t2);	/* 2^11  - 2^1	*/
	sqr25519(t3, t1);	/* 2^12  - 2^2	*/
	for (i = 1; i < 5; i++) {
		sqr25519(t1, t3);
		sqr25519(t3, t1);
	} /* t3 */		/* 2^20  - 2^10	*/
	mul25519(t1, t3, t2);	/* 2^20  - 2^0	*/
	sqr25519(t3, t1);	/* 2^21  - 2^1	*/
	sqr25519(t4, t3);	/* 2^22  - 2^2	*/
	for (i = 1; i < 10; i++) {
		sqr25519(t3, t4);
		sqr25519(t4, t3);
	} /* t4 */		/* 2^40  - 2^20	*/
	mul25519(t3, t4, t1);	/* 2^40  - 2^0	*/
	for (i = 0; i < 5; i++) {
		sqr25519(t1, t3);
		sqr25519(t3, t1);
	} /* t3 */		/* 2^50  - 2^10	*/
	mul25519(t1, t3, t2);	/* 2^50  - 2^0	*/
	sqr25519(t2, t1);	/* 2^51  - 2^1	*/
	sqr25519(t3, t2);	/* 2^52  - 2^2	*/
	for (i = 1; i < 25; i++) {
		sqr25519(t2, t3);
		sqr25519(t3, t2);
	} /* t3 */		/* 2^100 - 2^50 */
	mul25519(t2, t3, t1);	/* 2^100 - 2^0	*/
	sqr25519(t3, t2);	/* 2^101 - 2^1	*/
	sqr25519(t4, t3);	/* 2^102 - 2^2	*/
	for (i = 1; i < 50; i++) {
		sqr25519(t3, t4);
		sqr25519(t4, t3);
	} /* t4 */		/* 2^200 - 2^100 */
	mul25519(t3, t4, t2);	/* 2^200 - 2^0	*/
	for (i = 0; i < 25; i++) {
		sqr25519(t4, t3);
		sqr25519(t3, t4);
	} /* t3 */		/* 2^250 - 2^50	*/
	mul25519(t2, t3, t1);	/* 2^250 - 2^0	*/
	sqr25519(t1, t2);	/* 2^251 - 2^1	*/
	sqr25519(t2, t1);	/* 2^252 - 2^2	*/
	if (sqrtassist) {
		mul25519(y, x, t2);	/* 2^252 - 3 */
	} else {
		sqr25519(t1, t2);	/* 2^253 - 2^3	*/
		sqr25519(t2, t1);	/* 2^254 - 2^4	*/
		sqr25519(t1, t2);	/* 2^255 - 2^5	*/
		mul25519(y, t1, t0);	/* 2^255 - 21	*/
	}
}

/* checks if x is "negative", requires reduced input */
static inline int is_negative(i25519 x) {
	return (is_overflow(x) | (x[9] < 0)) ^ (x[0] & 1);
}

/* a square root */
static void sqrt25519(i25519 x, i25519 u) {
	i25519 v, t1, t2;
	add25519(t1, u, u); /* t1 = 2u    */
	recip25519(v, t1, 1); /* v = (2u)^((p-5)/8) */
	sqr25519(x, v); /* x = v^2    */
	mul25519(t2, t1, x); /* t2 = 2uv^2   */
	t2[0]--; /* t2 = 2uv^2-1   */
	mul25519(t1, v, t2); /* t1 = v(2uv^2-1)  */
	mul25519(x, u, t1); /* x = uv(2uv^2-1)  */
}

/********************* Elliptic curve *********************/

/* y^2 = x^3 + 486662 x^2 + x  over GF(2^255-19) */


/* t1 = ax + az
 * t2 = ax - az  */
static inline void mont_prep(i25519 t1, i25519 t2, i25519 ax, i25519 az) {
	add25519(t1, ax, az);
	sub25519(t2, ax, az);
}

/* A = P + Q   where
 *  X(A) = ax/az
 *  X(P) = (t1+t2)/(t1-t2)
 *  X(Q) = (t3+t4)/(t3-t4)
 *  X(P-Q) = dx
 * clobbers t1 and t2, preserves t3 and t4  */
static inline void mont_add(i25519 t1, i25519 t2, i25519 t3, i25519 t4,
			i25519 ax, i25519 az, const i25519 dx) {
	mul25519(ax, t2, t3);
	mul25519(az, t1, t4);
	add25519(t1, ax, az);
	sub25519(t2, ax, az);
	sqr25519(ax, t1);
	sqr25519(t1, t2);
	mul25519(az, t1, dx);
}

/* B = 2 * Q   where
 *  X(B) = bx/bz
 *  X(Q) = (t3+t4)/(t3-t4)
 * clobbers t1 and t2, preserves t3 and t4  */
static inline void mont_dbl(i25519 t1, i25519 t2, i25519 t3, i25519 t4,
			i25519 bx, i25519 bz) {
	sqr25519(t1, t3);
	sqr25519(t2, t4);
	mul25519(bx, t1, t2);
	sub25519(t2, t1, t2);
	mul25519small(bz, t2, 121665);
	add25519(t1, t1, bz);
	mul25519(bz, t1, t2);
}

/* Y^2 = X^3 + 486662 X^2 + X
 * t is a temporary  */
static inline void x_to_y2(i25519 t, i25519 y2, const i25519 x) {
	sqr25519(t, x);
	mul25519small(y2, x, 486662);
	add25519(t, t, y2);
	t[0]++;
	mul25519(y2, t, x);
}

/* P = kG   and  s = sign(P)/k */
void core25519(k25519 Px, k25519 s, const k25519 k, const k25519 Gx) {
	i25519 dx, x[2], z[2], t1, t2, t3, t4;
	unsigned i, j;

	/* unpack the base */
	if (Gx)
		unpack25519(dx, Gx);
	else
		set25519(dx, 9);

	/* 0G = point-at-infinity */
	set25519(x[0], 1);
	set25519(z[0], 0);

	/* 1G = G */
	cpy25519(x[1], dx);
	set25519(z[1], 1);

	for (i = 32; i--; ) {
		for (j = 8; j--; ) {
			/* swap arguments depending on bit */
			const int bit1 = k[i] >> j & 1;
			const int bit0 = ~k[i] >> j & 1;
			int32_t *const ax = x[bit0];
			int32_t *const az = z[bit0];
			int32_t *const bx = x[bit1];
			int32_t *const bz = z[bit1];

			/* a' = a + b	*/
			/* b' = 2 b	*/
			mont_prep(t1, t2, ax, az);
			mont_prep(t3, t4, bx, bz);
			mont_add(t1, t2, t3, t4, ax, az, dx);
			mont_dbl(t1, t2, t3, t4, bx, bz);
		}
	}

	recip25519(t1, z[0], 0);
	mul25519(dx, x[0], t1);
	pack25519(dx, Px);

	/* calculate s such that s abs(P) = G  .. assumes G is std base point */
	if (s) {
		x_to_y2(t2, t1, dx);	/* t1 = Py^2  */
		recip25519(t3, z[1], 0);	/* where Q=P+G ... */
		mul25519(t2, x[1], t3);	/* t2 = Qx  */
		add25519(t2, t2, dx);	/* t2 = Qx + Px  */
		t2[0] += 9 + 486662;	/* t2 = Qx + Px + Gx + 486662  */
		dx[0] -= 9;		/* dx = Px - Gx  */
		sqr25519(t3, dx);	/* t3 = (Px - Gx)^2  */
		mul25519(dx, t2, t3);	/* dx = t2 (Px - Gx)^2  */
		sub25519(dx, dx, t1);	/* dx = t2 (Px - Gx)^2 - Py^2  */
		dx[0] -= 39420360;	/* dx = t2 (Px - Gx)^2 - Py^2 - Gy^2  */
		mul25519(t1, dx, base_r2y);	/* t1 = -Py  */
		if (is_negative(t1))	/* sign is 1, so just copy  */
			cpy32(s, k);
		else			/* sign is -1, so negate  */
			mula_small(s, order_times_8, 0, k, 32, -1);

		/* reduce s mod q
		 * (is this needed?  do it just in case, it's fast anyway) */
		divmod((dstptr) t1, s, 32, order25519, 32);

		/* take reciprocal of s mod q */
		cpy32((dstptr) t1, order25519);
		cpy32(s, egcd32((dstptr) x, (dstptr) z, s, (dstptr) t1));
		if ((int8_t) s[31] < 0)
			mula_small(s, s, 0, order25519, 32, 1);
	}
}

/* v = (x - h) s mod q */
int sign25519(k25519 v, const k25519 h, const priv25519 x, const spriv25519 s) {
	int w, i;
	k25519 h1, x1;
	k25519 tmp1[2], tmp2[2], tmp3;

	// Don't clobber the arguments, be nice!
	cpy32(h1, h);
	cpy32(x1, x);

	// Reduce modulo group order
	divmod(tmp3, h1, 32, order25519, 32);
	divmod(tmp3, x1, 32, order25519, 32);

	// v = x1 - h1
	// If v is negative, add the group order to it to become positive.
	// If v was already positive we don't have to worry about overflow
	// when adding the order because v < order25519 and 2*order25519 < 2^256
	mula_small(v, x1, 0, h1, 32, -1);
	mula_small(v, v, 0, order25519, 32, 1);

	// tmp1 = (x-h)*s mod q
	memset(tmp1, 0, sizeof(tmp1));
	mula32((dstptr)tmp1, v, s, 32, 1);
	divmod((dstptr)tmp2, (dstptr)tmp1, 64, order25519, 32);

	for (w = 0, i = 0; i < 32; i++)
		w |= v[i] = tmp1[0][i];
	return w != 0;
}

/** Y = v abs(P) + h G  */
void verify25519(pub25519 Y, const k25519 v, const k25519 h, const pub25519 P) {
	k25519 d;
	i25519 p[2], s[2], yx[3], yz[3], t1[3], t2[3];

	int vi = 0, hi = 0, di = 0, nvh = 0;
	int i, j, k;

	/* set p[0] to G and p[1] to P  */

	set25519(p[0], 9);
	unpack25519(p[1], P);

	/* set s[0] to P+G and s[1] to P-G  */

	/* s[0] = (Py^2 + Gy^2 - 2 Py Gy)/(Px - Gx)^2 - Px - Gx - 486662  */
	/* s[1] = (Py^2 + Gy^2 + 2 Py Gy)/(Px - Gx)^2 - Px - Gx - 486662  */

	x_to_y2(t1[0], t2[0], p[1]); /* t2[0] = Py^2  */
	sqrt25519(t1[0], t2[0]); /* t1[0] = Py or -Py  */
	j = is_negative(t1[0]); /*      ... check which  */
	t2[0][0] += 39420360; /* t2[0] = Py^2 + Gy^2  */
	mul25519(t2[1], base_2y, t1[0]);/* t2[1] = 2 Py Gy or -2 Py Gy  */
	sub25519(t1[j], t2[0], t2[1]); /* t1[0] = Py^2 + Gy^2 - 2 Py Gy  */
	add25519(t1[1 - j], t2[0], t2[1]);/* t1[1] = Py^2 + Gy^2 + 2 Py Gy  */
	cpy25519(t2[0], p[1]); /* t2[0] = Px  */
	t2[0][0] -= 9; /* t2[0] = Px - Gx  */
	sqr25519(t2[1], t2[0]); /* t2[1] = (Px - Gx)^2  */
	recip25519(t2[0], t2[1], 0); /* t2[0] = 1/(Px - Gx)^2  */
	mul25519(s[0], t1[0], t2[0]); /* s[0] = t1[0]/(Px - Gx)^2  */
	sub25519(s[0], s[0], p[1]); /* s[0] = t1[0]/(Px - Gx)^2 - Px  */
	s[0][0] -= 9 + 486662; /* s[0] = X(P+G)  */
	mul25519(s[1], t1[1], t2[0]); /* s[1] = t1[1]/(Px - Gx)^2  */
	sub25519(s[1], s[1], p[1]); /* s[1] = t1[1]/(Px - Gx)^2 - Px  */
	s[1][0] -= 9 + 486662; /* s[1] = X(P-G)  */
	mul25519small(s[0], s[0], 1); /* reduce s[0] */
	mul25519small(s[1], s[1], 1); /* reduce s[1] */

	/* prepare the chain  */
	for (i = 0; i < 32; i++) {
		vi = (vi >> 8) ^ v[i] ^ (v[i] << 1);
		hi = (hi >> 8) ^ h[i] ^ (h[i] << 1);
		nvh = ~(vi ^ hi);
		di = (nvh & (di & 0x80) >> 7) ^ vi;
		di ^= nvh & (di & 0x01) << 1;
		di ^= nvh & (di & 0x02) << 1;
		di ^= nvh & (di & 0x04) << 1;
		di ^= nvh & (di & 0x08) << 1;
		di ^= nvh & (di & 0x10) << 1;
		di ^= nvh & (di & 0x20) << 1;
		di ^= nvh & (di & 0x40) << 1;
		d[i] = (unsigned char) di;
	}

	di = ((nvh & (di & 0x80) << 1) ^ vi) >> 8;

	/* initialize state */
	set25519(yx[0], 1);
	cpy25519(yx[1], p[di]);
	cpy25519(yx[2], s[0]);
	set25519(yz[0], 0);
	set25519(yz[1], 1);
	set25519(yz[2], 1);

	/* y[0] is (even)P + (even)G
	 * y[1] is (even)P + (odd)G  if current d-bit is 0
	 * y[1] is (odd)P + (even)G  if current d-bit is 1
	 * y[2] is (odd)P + (odd)G
	 */

	vi = 0;
	hi = 0;

	/* and go for it! */
	for (i = 32; i-- != 0; ) {
		vi = (vi << 8) | v[i];
		hi = (hi << 8) | h[i];
		di = (di << 8) | d[i];

		for (j = 8; j-- != 0; ) {
			mont_prep(t1[0], t2[0], yx[0], yz[0]);
			mont_prep(t1[1], t2[1], yx[1], yz[1]);
			mont_prep(t1[2], t2[2], yx[2], yz[2]);

			k = ((vi ^ vi >> 1) >> j & 1) + ((hi ^ hi >> 1) >> j & 1);
			mont_dbl(yx[2], yz[2], t1[k], t2[k], yx[0], yz[0]);

			k = (di >> j & 2) ^ ((di >> j & 1) << 1);
			mont_add(t1[1], t2[1], t1[k], t2[k], yx[1], yz[1], p[di >> j & 1]);

			mont_add(t1[2], t2[2], t1[0], t2[0], yx[2], yz[2], s[((vi ^ hi) >> j & 2) >> 1]);
		}
	}

	k = (vi & 1) + (hi & 1);
	recip25519(t1[0], yz[k], 0);
	mul25519(t1[1], yx[k], t1[0]);

	pack25519(t1[1], Y);
}

} // extern "C"