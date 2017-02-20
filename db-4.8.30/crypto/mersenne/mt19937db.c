/*
 * $Id$
 */
#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/hmac.h"

/* A C-program for MT19937: Integer version (1999/10/28)          */
/*  genrand() generates one pseudorandom unsigned integer (32bit) */
/* which is uniformly distributed among 0 to 2^32-1  for each     */
/* call. sgenrand(seed) sets initial values to the working area   */
/* of 624 words. Before genrand(), sgenrand(seed) must be         */
/* called once. (seed is any 32-bit integer.)                     */
/*   Coded by Takuji Nishimura, considering the suggestions by    */
/* Topher Cooper and Marc Rieffel in July-Aug. 1997.              */

/* This library is free software under the Artistic license:       */
/* see the file COPYING distributed together with this code.       */
/* For the verification of the code, its output sequence file      */
/* mt19937int.out is attached (2001/4/2)                           */

/* Copyright (C) 1997, 1999 Makoto Matsumoto and Takuji Nishimura. */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

/* REFERENCE                                                       */
/* M. Matsumoto and T. Nishimura,                                  */
/* "Mersenne Twister: A 623-Dimensionally Equidistributed Uniform  */
/* Pseudo-Random Number Generator",                                */
/* ACM Transactions on Modeling and Computer Simulation,           */
/* Vol. 8, No. 1, January 1998, pp 3--30.                          */

/* Period parameters */
#define	N 624
#define	M 397
#define	MATRIX_A 0x9908b0df   /* constant vector a */
#define	UPPER_MASK 0x80000000 /* most significant w-r bits */
#define	LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define	TEMPERING_MASK_B 0x9d2c5680
#define	TEMPERING_MASK_C 0xefc60000
#define	TEMPERING_SHIFT_U(y)  (y >> 11)
#define	TEMPERING_SHIFT_S(y)  (y << 7)
#define	TEMPERING_SHIFT_T(y)  (y << 15)
#define	TEMPERING_SHIFT_L(y)  (y >> 18)

static void __db_sgenrand __P((unsigned long, unsigned long *, int *));
#ifdef	NOT_USED
static void __db_lsgenrand __P((unsigned long *, unsigned long *, int *));
#endif
static unsigned long __db_genrand __P((ENV *));

/*
 * __db_generate_iv --
 *	Generate an initialization vector (IV)
 *
 * PUBLIC: int __db_generate_iv __P((ENV *, u_int32_t *));
 */
int
__db_generate_iv(env, iv)
	ENV *env;
	u_int32_t *iv;
{
	int i, n, ret;

	ret = 0;
	n = DB_IV_BYTES / sizeof(u_int32_t);
	MUTEX_LOCK(env, env->mtx_mt);
	if (env->mt == NULL) {
		if ((ret = __os_calloc(env, 1, N*sizeof(unsigned long),
		    &env->mt)) != 0)
			return (ret);
		/* mti==N+1 means mt[N] is not initialized */
		env->mti = N + 1;
	}
	for (i = 0; i < n; i++) {
		/*
		 * We do not allow 0.  If we get one just try again.
		 */
		do {
			iv[i] = (u_int32_t)__db_genrand(env);
		} while (iv[i] == 0);
	}

	MUTEX_UNLOCK(env, env->mtx_mt);
	return (0);
}

/* Initializing the array with a seed */
static void
__db_sgenrand(seed, mt, mtip)
	unsigned long seed;
	unsigned long mt[];
	int *mtip;
{
    int i;

    DB_ASSERT(NULL, seed != 0);
    for (i=0;i<N;i++) {
	 mt[i] = seed & 0xffff0000;
	 seed = 69069 * seed + 1;
	 mt[i] |= (seed & 0xffff0000) >> 16;
	 seed = 69069 * seed + 1;
    }
    *mtip = N;
}

#ifdef	NOT_USED
/* Initialization by "sgenrand()" is an example. Theoretically,      */
/* there are 2^19937-1 possible states as an intial state.           */
/* This function allows to choose any of 2^19937-1 ones.             */
/* Essential bits in "seed_array[]" is following 19937 bits:         */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]. */
/* (seed_array[0]&LOWER_MASK) is discarded.                          */
/* Theoretically,                                                    */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]  */
/* can take any values except all zeros.                             */
static void
__db_lsgenrand(seed_array, mt, mtip)
    unsigned long seed_array[];
    unsigned long mt[];
    int *mtip;
    /* the length of seed_array[] must be at least N */
{
    int i;

    for (i=0;i<N;i++)
      mt[i] = seed_array[i];
    *mtip=N;
}
#endif

static unsigned long
__db_genrand(env)
    ENV *env;
{
    db_timespec ts;
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    u_int32_t seed;

    /*
     * We are called with ENV->mtx_mt locked.
     */
    if (env->mti >= N) { /* generate N words at one time */
	int kk;

	if (env->mti == N+1) {  /* if sgenrand() has not been called, */
		/*
		 * Seed the generator with the hashed time.  The __db_mac
		 * function will return 4 bytes if we don't send in a key.
		 */
		do {
			__os_gettime(env, &ts, 1);
			__db_chksum(NULL, (u_int8_t *)&ts.tv_sec,
			    sizeof(ts.tv_sec), NULL, (u_int8_t *)&seed);
		} while (seed == 0);
		__db_sgenrand((unsigned long)seed, env->mt, &env->mti);
	}

	for (kk=0;kk<N-M;kk++) {
	    y = (env->mt[kk]&UPPER_MASK)|(env->mt[kk+1]&LOWER_MASK);
	    env->mt[kk] = env->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
	}
	for (;kk<N-1;kk++) {
	    y = (env->mt[kk]&UPPER_MASK)|(env->mt[kk+1]&LOWER_MASK);
	    env->mt[kk] = env->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
	}
	y = (env->mt[N-1]&UPPER_MASK)|(env->mt[0]&LOWER_MASK);
	env->mt[N-1] = env->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

	env->mti = 0;
    }

    y = env->mt[env->mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}
