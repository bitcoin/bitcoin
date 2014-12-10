/**********************************************************************
 * Copyright (c) 2013-2014 Diederik Huys, Pieter Wuille               *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

/**
 * Changelog:
 * - March 2013, Diederik Huys:    original version
 * - November 2014, Pieter Wuille: updated to use Peter Dettman's parallel multiplication algorithm
 * - December 2014, Pieter Wuille: converted from YASM to GCC inline assembly
 */

#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint64_t *r, const uint64_t *a, const uint64_t * SECP256K1_RESTRICT b) {
/**
 * Registers: rdx:rax = multiplication accumulator
 *            r9:r8   = c
 *            r15:rcx = d
 *            r10-r14 = a0-a4
 *            rbx     = b
 *            rdi     = r
 *            rsi     = a / t?
 */
  uint64_t tmp1, tmp2, tmp3;
__asm__ __volatile__(
    "movq 0(%%rsi),%%r10\n"
    "movq 8(%%rsi),%%r11\n"
    "movq 16(%%rsi),%%r12\n"
    "movq 24(%%rsi),%%r13\n"
    "movq 32(%%rsi),%%r14\n"

    /* d += a3 * b0 */
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "movq %%rax,%%rcx\n"
    "movq %%rdx,%%r15\n"
    /* d += a2 * b1 */
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a1 * b2 */
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d = a0 * b3 */
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* c = a4 * b4 */
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    /* d += (c & M) * R */
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* c >>= 52 (%%r8 only) */
    "shrdq $52,%%r9,%%r8\n"
    /* t3 (tmp1) = d & M */
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    "movq %%rsi,%q1\n"
    /* d >>= 52 */
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    /* d += a4 * b0 */
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a3 * b1 */
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a2 * b2 */
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a1 * b3 */
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a0 * b4 */
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += c * R */
    "movq %%r8,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* t4 = d & M (%%rsi) */
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    /* d >>= 52 */
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    /* tx = t4 >> 48 (tmp3) */
    "movq %%rsi,%%rax\n"
    "shrq $48,%%rax\n"
    "movq %%rax,%q3\n"
    /* t4 &= (M >> 4) (tmp2) */
    "movq $0xffffffffffff,%%rax\n"
    "andq %%rax,%%rsi\n"
    "movq %%rsi,%q2\n"
    /* c = a0 * b0 */
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    /* d += a4 * b1 */
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a3 * b2 */
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a2 * b3 */
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a1 * b4 */
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* u0 = d & M (%%rsi) */
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    /* d >>= 52 */
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    /* u0 = (u0 << 4) | tx (%%rsi) */
    "shlq $4,%%rsi\n"
    "movq %q3,%%rax\n"
    "orq %%rax,%%rsi\n"
    /* c += u0 * (R >> 4) */
    "movq $0x1000003d1,%%rax\n"
    "mulq %%rsi\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* r[0] = c & M */
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,0(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* c += a1 * b0 */
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* c += a0 * b1 */
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d += a4 * b2 */
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a3 * b3 */
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a2 * b4 */
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* c += (d & M) * R */
    "movq %%rcx,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d >>= 52 */
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    /* r[1] = c & M */
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,8(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* c += a2 * b0 */
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* c += a1 * b1 */
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* c += a0 * b2 (last use of %%r10 = a0) */
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* fetch t3 (%%r10, overwrites a0), t4 (%%rsi) */
    "movq %q2,%%rsi\n"
    "movq %q1,%%r10\n"
    /* d += a4 * b3 */
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* d += a3 * b4 */
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    /* c += (d & M) * R */
    "movq %%rcx,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d >>= 52 (%%rcx only) */
    "shrdq $52,%%r15,%%rcx\n"
    /* r[2] = c & M */
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,16(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* c += t3 */
    "addq %%r10,%%r8\n"
    /* c += d * R */
    "movq %%rcx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* r[3] = c & M */
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,24(%%rdi)\n"
    /* c >>= 52 (%%r8 only) */
    "shrdq $52,%%r9,%%r8\n"
    /* c += t4 (%%r8 only) */
    "addq %%rsi,%%r8\n"
    /* r[4] = c */
    "movq %%r8,32(%%rdi)\n"
: "+S"(a), "=m"(tmp1), "=m"(tmp2), "=m"(tmp3)
: "b"(b), "D"(r)
: "%rax", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "cc", "memory"
);
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint64_t *r, const uint64_t *a) {
/**
 * Registers: rdx:rax = multiplication accumulator
 *            r9:r8   = c
 *            rcx:rbx = d
 *            r10-r14 = a0-a4
 *            r15     = M (0xfffffffffffff)
 *            rdi     = r
 *            rsi     = a / t?
 */
  uint64_t tmp1, tmp2, tmp3;
__asm__ __volatile__(
    "movq 0(%%rsi),%%r10\n"
    "movq 8(%%rsi),%%r11\n"
    "movq 16(%%rsi),%%r12\n"
    "movq 24(%%rsi),%%r13\n"
    "movq 32(%%rsi),%%r14\n"
    "movq $0xfffffffffffff,%%r15\n"

    /* d = (a0*2) * a3 */
    "leaq (%%r10,%%r10,1),%%rax\n"
    "mulq %%r13\n"
    "movq %%rax,%%rbx\n"
    "movq %%rdx,%%rcx\n"
    /* d += (a1*2) * a2 */
    "leaq (%%r11,%%r11,1),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* c = a4 * a4 */
    "movq %%r14,%%rax\n"
    "mulq %%r14\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    /* d += (c & M) * R */
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* c >>= 52 (%%r8 only) */
    "shrdq $52,%%r9,%%r8\n"
    /* t3 (tmp1) = d & M */
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    "movq %%rsi,%q1\n"
    /* d >>= 52 */
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    /* a4 *= 2 */
    "addq %%r14,%%r14\n"
    /* d += a0 * a4 */
    "movq %%r10,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* d+= (a1*2) * a3 */
    "leaq (%%r11,%%r11,1),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* d += a2 * a2 */
    "movq %%r12,%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* d += c * R */
    "movq %%r8,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* t4 = d & M (%%rsi) */
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    /* d >>= 52 */
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    /* tx = t4 >> 48 (tmp3) */
    "movq %%rsi,%%rax\n"
    "shrq $48,%%rax\n"
    "movq %%rax,%q3\n"
    /* t4 &= (M >> 4) (tmp2) */
    "movq $0xffffffffffff,%%rax\n"
    "andq %%rax,%%rsi\n"
    "movq %%rsi,%q2\n"
    /* c = a0 * a0 */
    "movq %%r10,%%rax\n"
    "mulq %%r10\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    /* d += a1 * a4 */
    "movq %%r11,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* d += (a2*2) * a3 */
    "leaq (%%r12,%%r12,1),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* u0 = d & M (%%rsi) */
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    /* d >>= 52 */
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    /* u0 = (u0 << 4) | tx (%%rsi) */
    "shlq $4,%%rsi\n"
    "movq %q3,%%rax\n"
    "orq %%rax,%%rsi\n"
    /* c += u0 * (R >> 4) */
    "movq $0x1000003d1,%%rax\n"
    "mulq %%rsi\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* r[0] = c & M */
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,0(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* a0 *= 2 */
    "addq %%r10,%%r10\n"
    /* c += a0 * a1 */
    "movq %%r10,%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d += a2 * a4 */
    "movq %%r12,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* d += a3 * a3 */
    "movq %%r13,%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* c += (d & M) * R */
    "movq %%rbx,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d >>= 52 */
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    /* r[1] = c & M */
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,8(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* c += a0 * a2 (last use of %%r10) */
    "movq %%r10,%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* fetch t3 (%%r10, overwrites a0),t4 (%%rsi) */
    "movq %q2,%%rsi\n"
    "movq %q1,%%r10\n"
    /* c += a1 * a1 */
    "movq %%r11,%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d += a3 * a4 */
    "movq %%r13,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    /* c += (d & M) * R */
    "movq %%rbx,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* d >>= 52 (%%rbx only) */
    "shrdq $52,%%rcx,%%rbx\n"
    /* r[2] = c & M */
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,16(%%rdi)\n"
    /* c >>= 52 */
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    /* c += t3 */
    "addq %%r10,%%r8\n"
    /* c += d * R */
    "movq %%rbx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    /* r[3] = c & M */
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,24(%%rdi)\n"
    /* c >>= 52 (%%r8 only) */
    "shrdq $52,%%r9,%%r8\n"
    /* c += t4 (%%r8 only) */
    "addq %%rsi,%%r8\n"
    /* r[4] = c */
    "movq %%r8,32(%%rdi)\n"
: "+S"(a), "=m"(tmp1), "=m"(tmp2), "=m"(tmp3)
: "D"(r)
: "%rax", "%rbx", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "cc", "memory"
);
}

#endif
