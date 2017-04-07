/*-
 */
#ifndef _CRYPTO_SCRYPT_H_
#define _CRYPTO_SCRYPT_H_


#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C"{
#endif

/**
 * crypto_scrypt(passwd, passwdlen, salt, saltlen, N, r, p, buf, buflen):
 * Compute scrypt(passwd[0 .. passwdlen - 1], salt[0 .. saltlen - 1], N, r,
 * p, buflen) and write the result into buf.  The parameters r, p, and buflen
 * must satisfy r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter N
 * must be a power of 2 greater than 1.
 *
 * libscrypt_scrypt(passwd, passwdlen, salt, saltlen, N, r, p, buf, buflen):
 * password; duh
 * N: CPU AND RAM cost (first modifier)
 * r: RAM Cost
 * p: CPU cost (parallelisation)
 * In short, N is your main performance modifier. Values of r = 8, p = 1 are
 * standard unless you want to modify the CPU/RAM ratio.
 * Return 0 on success; or -1 on error.
 */
int libscrypt_scrypt(const uint8_t *, size_t, const uint8_t *, size_t, uint64_t,
    uint32_t, uint32_t, /*@out@*/ uint8_t *, size_t);

/* Converts a series of input parameters to a MCF form for storage */
int libscrypt_mcf(uint32_t N, uint32_t r, uint32_t p, const char *salt,
	const char *hash, char *mcf);

#ifndef _MSC_VER
/* Generates a salt. Uses /dev/urandom/
 */
int libscrypt_salt_gen(/*@out@*/ uint8_t *rand, size_t len);

/* Creates a hash of a passphrase using a randomly generated salt */
/* Returns >0 on success, or 0 for fail */
int libscrypt_hash(char *dst, const char* passphrase, uint32_t N, uint8_t r,
  uint8_t p);
#endif

/* Checks a given MCF against a password */
int libscrypt_check(char *mcf, const char *password);

#ifdef __cplusplus
}
#endif

/* Sane default values */
#define SCRYPT_HASH_LEN 64 /* This can be user defined - 
 *but 64 is the reference size
 */
#define SCRYPT_SAFE_N 30 /* This is much higher than you want. It's just
			  * a blocker for insane defines
			  */
#define SCRYPT_SALT_LEN 16 /* This is just a recommended size */
/* Standard MCF is:
   $s1 Identifier, three chars
   $0e0810 Work order and separator, six chars
   Formula for binary to base64 length = ceil(n/3)*4
   $pcL+DWle903AXcKJVwMffA== Salt is 16 bytes, or 24 in Base64
   $dn+9ujljVc5JTJMC2fYu1ZEHdJyqYkOurmcrBQbMHUfnD6qxbTmNiR075ohNBZjvp66E2aV1pfOrmyNHUefjMg== Hash is 64 bytes, or 88 in Base64.
   Work order, salt and hash have separators (3)
   3 + 6 + 24 + 88 + 3 + null byte = 25
   This is rounded up to a multiple of four for alignment
*/
#define SCRYPT_MCF_LEN 128
#define SCRYPT_MCF_ID "$s1"
#define SCRYPT_N 16384
#define SCRYPT_r 8
#define SCRYPT_p 16
#endif /* !_CRYPTO_SCRYPT_H_ */
