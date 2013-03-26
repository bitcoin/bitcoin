#ifndef _SECP256K1_LIN64
#define _SECP256K1_LIN64

#ifdef INLINE_ASM
extern "C" void _ExSetMult(uint64_t *, uint64_t *, uint64_t *);
extern "C" void _ExSetSquare(uint64_t *, uint64_t *);
#endif

#endif
