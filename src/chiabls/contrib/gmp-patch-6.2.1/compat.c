/* Old function entrypoints retained for binary compatibility.

Copyright 2000, 2001 Free Software Foundation, Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the GNU MP Library.  If not,
see https://www.gnu.org/licenses/.  */

#include <stdio.h>
#include "gmp-impl.h"

/* RUNTIMECPUID */
int bCheckedBMI = 0;
int bBMI1 = 0;
int bBMI2 = 0;
int bCheckedLZCNT = 0;
int bLZCNT = 0;

/* mpn_divexact_by3 was a function in gmp 3.0.1, but as of gmp 3.1 it's a
   macro calling mpn_divexact_by3c.  */
mp_limb_t
__MPN (divexact_by3) (mp_ptr dst, mp_srcptr src, mp_size_t size)
{
  return mpn_divexact_by3 (dst, src, size);
}


/* mpn_divmod_1 was a function in gmp 3.0.1 and earlier, but marked obsolete
   in both gmp 2 and 3.  As of gmp 3.1 it's a macro calling mpn_divrem_1. */
mp_limb_t
__MPN (divmod_1) (mp_ptr dst, mp_srcptr src, mp_size_t size, mp_limb_t divisor)
{
  return mpn_divmod_1 (dst, src, size, divisor);
}


/* mpz_legendre was a separate function in gmp 3.1.1 and earlier, but as of
   4.0 it's a #define alias for mpz_jacobi.  */
int
__gmpz_legendre (mpz_srcptr a, mpz_srcptr b)
{
  return mpz_jacobi (a, b);
}
