/* hash.c     April 2012
 * Groestl ANSI C code optimised for 32-bit machines
 * Author: Thomas Krinninger
 *
 *  This work is based on the implementation of
 *          Soeren S. Thomsen and Krystian Matusiewicz
 *          
 *
 */

#include <crypto/c_groestl.h>
#include <crypto/groestl_tables.h>

#define P_TYPE 0
#define Q_TYPE 1

const uint8_t shift_Values[2][8] = {{0,1,2,3,4,5,6,7},{1,3,5,7,0,2,4,6}};

const uint8_t indices_cyclic[15] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6};


#define ROTATE_COLUMN_DOWN(v1, v2, amount_bytes, temp_var) {temp_var = (v1<<(8*amount_bytes))|(v2>>(8*(4-amount_bytes))); \
															v2 = (v2<<(8*amount_bytes))|(v1>>(8*(4-amount_bytes))); \
															v1 = temp_var;}
  

#define COLUMN(x,y,i,c0,c1,c2,c3,c4,c5,c6,c7,tv1,tv2,tu,tl,t)				\
   tu = T[2*(uint32_t)x[4*c0+0]];			    \
   tl = T[2*(uint32_t)x[4*c0+0]+1];		    \
   tv1 = T[2*(uint32_t)x[4*c1+1]];			\
   tv2 = T[2*(uint32_t)x[4*c1+1]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,1,t)	\
   tu ^= tv1;						\
   tl ^= tv2;						\
   tv1 = T[2*(uint32_t)x[4*c2+2]];			\
   tv2 = T[2*(uint32_t)x[4*c2+2]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,2,t)	\
   tu ^= tv1;						\
   tl ^= tv2;   					\
   tv1 = T[2*(uint32_t)x[4*c3+3]];			\
   tv2 = T[2*(uint32_t)x[4*c3+3]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,3,t)	\
   tu ^= tv1;						\
   tl ^= tv2;						\
   tl ^= T[2*(uint32_t)x[4*c4+0]];			\
   tu ^= T[2*(uint32_t)x[4*c4+0]+1];			\
   tv1 = T[2*(uint32_t)x[4*c5+1]];			\
   tv2 = T[2*(uint32_t)x[4*c5+1]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,1,t)	\
   tl ^= tv1;						\
   tu ^= tv2;						\
   tv1 = T[2*(uint32_t)x[4*c6+2]];			\
   tv2 = T[2*(uint32_t)x[4*c6+2]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,2,t)	\
   tl ^= tv1;						\
   tu ^= tv2;   					\
   tv1 = T[2*(uint32_t)x[4*c7+3]];			\
   tv2 = T[2*(uint32_t)x[4*c7+3]+1];			\
   ROTATE_COLUMN_DOWN(tv1,tv2,3,t)	\
   tl ^= tv1;						\
   tu ^= tv2;						\
   y[i] = tu;						\
   y[i+1] = tl;


/* compute one round of P (short variants) */
static void RND512P(uint8_t *x, uint32_t *y, uint32_t r) {
  uint32_t temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp;
  uint32_t* x32 = (uint32_t*)x;
  x32[ 0] ^= 0x00000000^r;
  x32[ 2] ^= 0x00000010^r;
  x32[ 4] ^= 0x00000020^r;
  x32[ 6] ^= 0x00000030^r;
  x32[ 8] ^= 0x00000040^r;
  x32[10] ^= 0x00000050^r;
  x32[12] ^= 0x00000060^r;
  x32[14] ^= 0x00000070^r;
  COLUMN(x,y, 0,  0,  2,  4,  6,  9, 11, 13, 15, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 2,  2,  4,  6,  8, 11, 13, 15,  1, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 4,  4,  6,  8, 10, 13, 15,  1,  3, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 6,  6,  8, 10, 12, 15,  1,  3,  5, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 8,  8, 10, 12, 14,  1,  3,  5,  7, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,10, 10, 12, 14,  0,  3,  5,  7,  9, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,12, 12, 14,  0,  2,  5,  7,  9, 11, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,14, 14,  0,  2,  4,  7,  9, 11, 13, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
}

/* compute one round of Q (short variants) */
static void RND512Q(uint8_t *x, uint32_t *y, uint32_t r) {
  uint32_t temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp;
  uint32_t* x32 = (uint32_t*)x;
  x32[ 0] = ~x32[ 0];
  x32[ 1] ^= 0xffffffff^r;
  x32[ 2] = ~x32[ 2];
  x32[ 3] ^= 0xefffffff^r;
  x32[ 4] = ~x32[ 4];
  x32[ 5] ^= 0xdfffffff^r;
  x32[ 6] = ~x32[ 6];
  x32[ 7] ^= 0xcfffffff^r;
  x32[ 8] = ~x32[ 8];
  x32[ 9] ^= 0xbfffffff^r;
  x32[10] = ~x32[10];
  x32[11] ^= 0xafffffff^r;
  x32[12] = ~x32[12];
  x32[13] ^= 0x9fffffff^r;
  x32[14] = ~x32[14];
  x32[15] ^= 0x8fffffff^r;
  COLUMN(x,y, 0,  2,  6, 10, 14,  1,  5,  9, 13, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 2,  4,  8, 12,  0,  3,  7, 11, 15, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 4,  6, 10, 14,  2,  5,  9, 13,  1, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 6,  8, 12,  0,  4,  7, 11, 15,  3, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y, 8, 10, 14,  2,  6,  9, 13,  1,  5, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,10, 12,  0,  4,  8, 11, 15,  3,  7, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,12, 14,  2,  6, 10, 13,  1,  5,  9, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
  COLUMN(x,y,14,  0,  4,  8, 12, 15,  3,  7, 11, temp_v1, temp_v2, temp_upper_value, temp_lower_value, temp);
}

/* compute compression function (short variants) */
static void F512(uint32_t *h, const uint32_t *m) {
  int i;
  uint32_t Ptmp[2*COLS512];
  uint32_t Qtmp[2*COLS512];
  uint32_t y[2*COLS512];
  uint32_t z[2*COLS512];

  for (i = 0; i < 2*COLS512; i++) {
    z[i] = m[i];
    Ptmp[i] = h[i]^m[i];
  }

  /* compute Q(m) */
  RND512Q((uint8_t*)z, y, 0x00000000);
  RND512Q((uint8_t*)y, z, 0x01000000);
  RND512Q((uint8_t*)z, y, 0x02000000);
  RND512Q((uint8_t*)y, z, 0x03000000);
  RND512Q((uint8_t*)z, y, 0x04000000);
  RND512Q((uint8_t*)y, z, 0x05000000);
  RND512Q((uint8_t*)z, y, 0x06000000);
  RND512Q((uint8_t*)y, z, 0x07000000);
  RND512Q((uint8_t*)z, y, 0x08000000);
  RND512Q((uint8_t*)y, Qtmp, 0x09000000);

  /* compute P(h+m) */
  RND512P((uint8_t*)Ptmp, y, 0x00000000);
  RND512P((uint8_t*)y, z, 0x00000001);
  RND512P((uint8_t*)z, y, 0x00000002);
  RND512P((uint8_t*)y, z, 0x00000003);
  RND512P((uint8_t*)z, y, 0x00000004);
  RND512P((uint8_t*)y, z, 0x00000005);
  RND512P((uint8_t*)z, y, 0x00000006);
  RND512P((uint8_t*)y, z, 0x00000007);
  RND512P((uint8_t*)z, y, 0x00000008);
  RND512P((uint8_t*)y, Ptmp, 0x00000009);

  /* compute P(h+m) + Q(m) + h */
  for (i = 0; i < 2*COLS512; i++) {
    h[i] ^= Ptmp[i]^Qtmp[i];
  }
}


/* digest up to msglen bytes of input (full blocks only) */
static void Transform(groestlHashState *ctx,
	       const uint8_t *input, 
	       int msglen) {

  /* digest message, one block at a time */
  for (; msglen >= SIZE512; 
       msglen -= SIZE512, input += SIZE512) {
    F512(ctx->chaining,(uint32_t*)input);

    /* increment block counter */
    ctx->block_counter1++;
    if (ctx->block_counter1 == 0) ctx->block_counter2++;
  }
}

/* given state h, do h <- P(h)+h */
static void OutputTransformation(groestlHashState *ctx) {
  int j;
  uint32_t temp[2*COLS512];
  uint32_t y[2*COLS512];
  uint32_t z[2*COLS512];



	for (j = 0; j < 2*COLS512; j++) {
	  temp[j] = ctx->chaining[j];
	}
	RND512P((uint8_t*)temp, y, 0x00000000);
	RND512P((uint8_t*)y, z, 0x00000001);
	RND512P((uint8_t*)z, y, 0x00000002);
	RND512P((uint8_t*)y, z, 0x00000003);
	RND512P((uint8_t*)z, y, 0x00000004);
	RND512P((uint8_t*)y, z, 0x00000005);
	RND512P((uint8_t*)z, y, 0x00000006);
	RND512P((uint8_t*)y, z, 0x00000007);
	RND512P((uint8_t*)z, y, 0x00000008);
	RND512P((uint8_t*)y, temp, 0x00000009);
	for (j = 0; j < 2*COLS512; j++) {
	  ctx->chaining[j] ^= temp[j];
	}									  
}

/* initialise context */
static void Init(groestlHashState* ctx) {
  int i = 0;
  /* allocate memory for state and data buffer */

  for(;i<(SIZE512/sizeof(uint32_t));i++)
  {
	ctx->chaining[i] = 0;
  }

  /* set initial value */
  ctx->chaining[2*COLS512-1] = u32BIG((uint32_t)HASH_BIT_LEN);

  /* set other variables */
  ctx->buf_ptr = 0;
  ctx->block_counter1 = 0;
  ctx->block_counter2 = 0;
  ctx->bits_in_last_byte = 0;
}

/* update state with databitlen bits of input */
static void Update(groestlHashState* ctx,
		  const BitSequence* input,
		  DataLength databitlen) {
  int index = 0;
  int msglen = (int)(databitlen/8);
  int rem = (int)(databitlen%8);

  /* if the buffer contains data that has not yet been digested, first
     add data to buffer until full */
  if (ctx->buf_ptr) {
    while (ctx->buf_ptr < SIZE512 && index < msglen) {
      ctx->buffer[(int)ctx->buf_ptr++] = input[index++];
    }
    if (ctx->buf_ptr < SIZE512) {
      /* buffer still not full, return */
      if (rem) {
	ctx->bits_in_last_byte = rem;
	ctx->buffer[(int)ctx->buf_ptr++] = input[index];
      }
      return;
    }

    /* digest buffer */
    ctx->buf_ptr = 0;
    Transform(ctx, ctx->buffer, SIZE512);
  }

  /* digest bulk of message */
  Transform(ctx, input+index, msglen-index);
  index += ((msglen-index)/SIZE512)*SIZE512;

  /* store remaining data in buffer */
  while (index < msglen) {
    ctx->buffer[(int)ctx->buf_ptr++] = input[index++];
  }

  /* if non-integral number of bytes have been supplied, store
     remaining bits in last byte, together with information about
     number of bits */
  if (rem) {
    ctx->bits_in_last_byte = rem;
    ctx->buffer[(int)ctx->buf_ptr++] = input[index];
  }
}

#define BILB ctx->bits_in_last_byte

/* finalise: process remaining data (including padding), perform
   output transformation, and write hash result to 'output' */
static void Final(groestlHashState* ctx,
		 BitSequence* output) {
  int i, j = 0, hashbytelen = HASH_BIT_LEN/8;
  uint8_t *s = (BitSequence*)ctx->chaining;

  /* pad with '1'-bit and first few '0'-bits */
  if (BILB) {
    ctx->buffer[(int)ctx->buf_ptr-1] &= ((1<<BILB)-1)<<(8-BILB);
    ctx->buffer[(int)ctx->buf_ptr-1] ^= 0x1<<(7-BILB);
    BILB = 0;
  }
  else ctx->buffer[(int)ctx->buf_ptr++] = 0x80;

  /* pad with '0'-bits */
  if (ctx->buf_ptr > SIZE512-LENGTHFIELDLEN) {
    /* padding requires two blocks */
    while (ctx->buf_ptr < SIZE512) {
      ctx->buffer[(int)ctx->buf_ptr++] = 0;
    }
    /* digest first padding block */
    Transform(ctx, ctx->buffer, SIZE512);
    ctx->buf_ptr = 0;
  }
  while (ctx->buf_ptr < SIZE512-LENGTHFIELDLEN) {
    ctx->buffer[(int)ctx->buf_ptr++] = 0;
  }

  /* length padding */
  ctx->block_counter1++;
  if (ctx->block_counter1 == 0) ctx->block_counter2++;
  ctx->buf_ptr = SIZE512;

  while (ctx->buf_ptr > SIZE512-(int)sizeof(uint32_t)) {
    ctx->buffer[(int)--ctx->buf_ptr] = (uint8_t)ctx->block_counter1;
    ctx->block_counter1 >>= 8;
  }
  while (ctx->buf_ptr > SIZE512-LENGTHFIELDLEN) {
    ctx->buffer[(int)--ctx->buf_ptr] = (uint8_t)ctx->block_counter2;
    ctx->block_counter2 >>= 8;
  }
  /* digest final padding block */
  Transform(ctx, ctx->buffer, SIZE512); 
  /* perform output transformation */
  OutputTransformation(ctx);

  /* store hash result in output */
  for (i = SIZE512-hashbytelen; i < SIZE512; i++,j++) {
    output[j] = s[i];
  }

  /* zeroise relevant variables and deallocate memory */
  for (i = 0; i < COLS512; i++) {
    ctx->chaining[i] = 0;
  }
  for (i = 0; i < SIZE512; i++) {
    ctx->buffer[i] = 0;
  }
}

/* hash bit sequence */
void groestl(const BitSequence* data, 
		DataLength databitlen,
		BitSequence* hashval) {

  groestlHashState context;

  /* initialise */
    Init(&context);


  /* process message */
  Update(&context, data, databitlen);

  /* finalise */
  Final(&context, hashval);
}
/*
static int crypto_hash(unsigned char *out,
		const unsigned char *in,
		unsigned long long len)
{
  groestl(in, 8*len, out);
  return 0;
}

*/
