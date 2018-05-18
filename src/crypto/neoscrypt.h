#if (__cplusplus)
extern "C" {
#endif

void neoscrypt(const unsigned char *password, unsigned char *output,
  unsigned int profile);

void neoscrypt_blake2s(const void *input, const unsigned int input_size,
  const void *key, const unsigned char key_size,
  void *output, const unsigned char output_size);

void neoscrypt_copy(void *dstp, const void *srcp, unsigned int len);
void neoscrypt_erase(void *dstp, unsigned int len);
void neoscrypt_xor(void *dstp, const void *srcp, unsigned int len);

#if defined(ASM) && defined(MINER_4WAY)
void neoscrypt_4way(const unsigned char *password, unsigned char *output,
  unsigned char *scratchpad);

#ifdef SHA256
void scrypt_4way(const unsigned char *password, unsigned char *output,
  unsigned char *scratchpad);
#endif

void neoscrypt_blake2s_4way(const unsigned char *input,
  const unsigned char *key, unsigned char *output);

void neoscrypt_fastkdf_4way(const unsigned char *password,
  const unsigned char *salt, unsigned char *output, unsigned char *scratchpad,
  const unsigned int mode);
#endif

unsigned int cpu_vec_exts(void);

#if (__cplusplus)
}
#else

typedef unsigned long long ullong;
typedef signed long long llong;
typedef unsigned int uint;
typedef unsigned char uchar;

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? a : b)
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? a : b)
#endif

#define BLOCK_SIZE 64
#define DIGEST_SIZE 32

typedef uchar hash_digest[DIGEST_SIZE];

#define ROTL32(a,b) (((a) << (b)) | ((a) >> (32 - b)))
#define ROTR32(a,b) (((a) >> (b)) | ((a) << (32 - b)))

#define U8TO32_BE(p) \
    (((uint)((p)[0]) << 24) | ((uint)((p)[1]) << 16) | \
    ((uint)((p)[2]) <<  8) | ((uint)((p)[3])))

#define U32TO8_BE(p, v) \
    (p)[0] = (uchar)((v) >> 24); (p)[1] = (uchar)((v) >> 16); \
    (p)[2] = (uchar)((v) >>  8); (p)[3] = (uchar)((v)      );

#define U64TO8_BE(p, v) \
    U32TO8_BE((p),     (uint)((v) >> 32)); \
    U32TO8_BE((p) + 4, (uint)((v)      ));

#endif
