#ifndef __hash_h
#define __hash_h
/*
#include "crypto_uint8.h"
#include "crypto_uint32.h"
#include "crypto_uint64.h"
#include "crypto_hash.h" 

typedef crypto_uint8 uint8_t; 
typedef crypto_uint32 uint32_t; 
typedef crypto_uint64 uint64_t;
*/
#include <stdint.h>
#include <crypto/types.h>

/* some sizes (number of bytes) */
#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8

#define SIZE512 (ROWS*COLS512)

#define ROUNDS512 10
#define HASH_BIT_LEN 256

#define ROTL32(v, n) ((((v)<<(n))|((v)>>(32-(n))))&li_32(ffffffff))


#define li_32(h) 0x##h##u
#define EXT_BYTE(var,n) ((uint8_t)((uint32_t)(var) >> (8*n)))
#define u32BIG(a)				\
  ((ROTL32(a,8) & li_32(00FF00FF)) |		\
   (ROTL32(a,24) & li_32(FF00FF00)))


/* NIST API begin */
typedef struct {
  uint32_t chaining[SIZE512/sizeof(uint32_t)];            /* actual state */
  uint32_t block_counter1,
  block_counter2;         /* message block counter(s) */
  BitSequence buffer[SIZE512];      /* data buffer */
  int buf_ptr;              /* data buffer pointer */
  int bits_in_last_byte;    /* no. of message bits in last byte of
			       data buffer */
} groestlHashState;

/*void Init(hashState*);
void Update(hashState*, const BitSequence*, DataLength);
void Final(hashState*, BitSequence*); */
void groestl(const BitSequence*, DataLength, BitSequence*);
/* NIST API end   */

/*
int crypto_hash(unsigned char *out,
		const unsigned char *in,
		unsigned long long len);
*/

#endif /* __hash_h */
