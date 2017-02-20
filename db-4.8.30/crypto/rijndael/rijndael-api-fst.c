/**
 * rijndael-api-fst.c
 *
 * @version 2.9 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Acknowledgements:
 *
 * We are deeply indebted to the following people for their bug reports,
 * fixes, and improvement suggestions to this implementation. Though we
 * tried to list all contributions, we apologise in advance for any
 * missing reference.
 *
 * Andrew Bales <Andrew.Bales@Honeywell.com>
 * Markus Friedl <markus.friedl@informatik.uni-erlangen.de>
 * John Skodon <skodonj@webquill.com>
 */
#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"

#include "crypto/rijndael/rijndael-alg-fst.h"
#include "crypto/rijndael/rijndael-api-fst.h"

/*
 * __db_makeKey --
 *
 * PUBLIC: int __db_makeKey __P((keyInstance *, int, int, char *));
 */
int
__db_makeKey(key, direction, keyLen, keyMaterial)
	keyInstance *key;
	int direction;
	int keyLen;
	char *keyMaterial;
{
	u8 cipherKey[MAXKB];

	if (key == NULL) {
		return BAD_KEY_INSTANCE;
	}

	if ((direction == DIR_ENCRYPT) || (direction == DIR_DECRYPT)) {
		key->direction = direction;
	} else {
		return BAD_KEY_DIR;
	}

	if ((keyLen == 128) || (keyLen == 192) || (keyLen == 256)) {
		key->keyLen = keyLen;
	} else {
		return BAD_KEY_MAT;
	}

	if (keyMaterial != NULL) {
		memcpy(cipherKey, keyMaterial, key->keyLen/8);
	}

	if (direction == DIR_ENCRYPT) {
		key->Nr = __db_rijndaelKeySetupEnc(key->rk, cipherKey, keyLen);
	} else {
		key->Nr = __db_rijndaelKeySetupDec(key->rk, cipherKey, keyLen);
	}
	__db_rijndaelKeySetupEnc(key->ek, cipherKey, keyLen);
	return TRUE;
}

/*
 * __db_cipherInit --
 *
 * PUBLIC: int __db_cipherInit __P((cipherInstance *, int, char *));
 */
int
__db_cipherInit(cipher, mode, IV)
	cipherInstance *cipher;
	int mode;
	char *IV;
{
	if ((mode == MODE_ECB) || (mode == MODE_CBC) || (mode == MODE_CFB1)) {
		cipher->mode = mode;
	} else {
		return BAD_CIPHER_MODE;
	}
	if (IV != NULL) {
	  memcpy(cipher->IV, IV, MAX_IV_SIZE);
	}
	return TRUE;
}

/*
 * __db_blockEncrypt --
 *
 * PUBLIC: int __db_blockEncrypt __P((cipherInstance *, keyInstance *, u_int8_t *,
 * PUBLIC:    size_t, u_int8_t *));
 */
int
__db_blockEncrypt(cipher, key, input, inputLen, outBuffer)
	cipherInstance *cipher;
	keyInstance *key;
	u_int8_t *input;
	size_t inputLen;
	u_int8_t *outBuffer;
{
	int i, k, t, numBlocks;
	u8 block[16], *iv;
	u32 tmpiv[4];

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_DECRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = (int)(inputLen/128);

	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			__db_rijndaelEncrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;

	case MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			memcpy(tmpiv, iv, MAX_IV_SIZE);
			((u32*)block)[0] = ((u32*)input)[0] ^ tmpiv[0];
			((u32*)block)[1] = ((u32*)input)[1] ^ tmpiv[1];
			((u32*)block)[2] = ((u32*)input)[2] ^ tmpiv[2];
			((u32*)block)[3] = ((u32*)input)[3] ^ tmpiv[3];
			__db_rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
			input += 16;
			outBuffer += 16;
		}
		break;

    case MODE_CFB1:
		iv = cipher->IV;
	for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
	    for (k = 0; k < 128; k++) {
				__db_rijndaelEncrypt(key->ek, key->Nr, iv, block);
		outBuffer[k >> 3] ^= (block[0] & (u_int)0x80) >> (k & 7);
		for (t = 0; t < 15; t++) {
			iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
		}
		iv[15] = (iv[15] << 1) | ((outBuffer[k >> 3] >> (7 - (k & 7))) & 1);
	    }
	    outBuffer += 16;
	    input += 16;
	}
	break;

	default:
		return BAD_CIPHER_STATE;
	}

	return 128*numBlocks;
}

/**
 * Encrypt data partitioned in octets, using RFC 2040-like padding.
 *
 * @param   input           data to be encrypted (octet sequence)
 * @param   inputOctets		input length in octets (not bits)
 * @param   outBuffer       encrypted output data
 *
 * @return	length in octets (not bits) of the encrypted output buffer.
 */
/*
 * __db_padEncrypt --
 *
 * PUBLIC: int __db_padEncrypt __P((cipherInstance *, keyInstance *, u_int8_t *,
 * PUBLIC:    int, u_int8_t *));
 */
int
__db_padEncrypt(cipher, key, input, inputOctets, outBuffer)
	cipherInstance *cipher;
	keyInstance *key;
	u_int8_t *input;
	int inputOctets;
	u_int8_t *outBuffer;
{
	int i, numBlocks, padLen;
	u8 block[16], *iv;
	u32 tmpiv[4];

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_DECRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			__db_rijndaelEncrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		DB_ASSERT(NULL, padLen > 0 && padLen <= 16);
		memcpy(block, input, 16 - padLen);
		memset(block + 16 - padLen, padLen, padLen);
		__db_rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;

	case MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			memcpy(tmpiv, iv, MAX_IV_SIZE);
			((u32*)block)[0] = ((u32*)input)[0] ^ tmpiv[0];
			((u32*)block)[1] = ((u32*)input)[1] ^ tmpiv[1];
			((u32*)block)[2] = ((u32*)input)[2] ^ tmpiv[2];
			((u32*)block)[3] = ((u32*)input)[3] ^ tmpiv[3];
			__db_rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		DB_ASSERT(NULL, padLen > 0 && padLen <= 16);
		for (i = 0; i < 16 - padLen; i++) {
			block[i] = input[i] ^ iv[i];
		}
		for (i = 16 - padLen; i < 16; i++) {
			block[i] = (u_int8_t)padLen ^ iv[i];
		}
		__db_rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;

	default:
		return BAD_CIPHER_STATE;
	}

	return 16*(numBlocks + 1);
}

/*
 * __db_blockDecrypt --
 *
 * PUBLIC: int __db_blockDecrypt __P((cipherInstance *, keyInstance *, u_int8_t *,
 * PUBLIC:    size_t, u_int8_t *));
 */
int
__db_blockDecrypt(cipher, key, input, inputLen, outBuffer)
	cipherInstance *cipher;
	keyInstance *key;
	u_int8_t *input;
	size_t inputLen;
	u_int8_t *outBuffer;
{
	int i, k, t, numBlocks;
	u8 block[16], *iv;
	u32 tmpiv[4];

	if (cipher == NULL ||
		key == NULL ||
		(cipher->mode != MODE_CFB1 && key->direction == DIR_ENCRYPT)) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = (int)(inputLen/128);

	switch (cipher->mode) {
	case MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			__db_rijndaelDecrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;

	case MODE_CBC:
		memcpy(tmpiv, cipher->IV, MAX_IV_SIZE);
		for (i = numBlocks; i > 0; i--) {
			__db_rijndaelDecrypt(key->rk, key->Nr, input, block);
			((u32*)block)[0] ^= tmpiv[0];
			((u32*)block)[1] ^= tmpiv[1];
			((u32*)block)[2] ^= tmpiv[2];
			((u32*)block)[3] ^= tmpiv[3];
			memcpy(tmpiv, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		break;

    case MODE_CFB1:
		iv = cipher->IV;
	for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
	    for (k = 0; k < 128; k++) {
				__db_rijndaelEncrypt(key->ek, key->Nr, iv, block);
		for (t = 0; t < 15; t++) {
			iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
		}
		iv[15] = (iv[15] << 1) | ((input[k >> 3] >> (7 - (k & 7))) & 1);
		outBuffer[k >> 3] ^= (block[0] & (u_int)0x80) >> (k & 7);
	    }
	    outBuffer += 16;
	    input += 16;
	}
	break;

	default:
		return BAD_CIPHER_STATE;
	}

	return 128*numBlocks;
}

/*
 * __db_padDecrypt --
 *
 * PUBLIC: int __db_padDecrypt __P((cipherInstance *, keyInstance *, u_int8_t *,
 * PUBLIC:    int, u_int8_t *));
 */
int
__db_padDecrypt(cipher, key, input, inputOctets, outBuffer)
	cipherInstance *cipher;
	keyInstance *key;
	u_int8_t *input;
	int inputOctets;
	u_int8_t *outBuffer;
{
	int i, numBlocks, padLen;
	u8 block[16];
	u32 tmpiv[4];

	if (cipher == NULL ||
		key == NULL ||
		key->direction == DIR_ENCRYPT) {
		return BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}
	if (inputOctets % 16 != 0) {
		return BAD_DATA;
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
	case MODE_ECB:
		/* all blocks but last */
		for (i = numBlocks - 1; i > 0; i--) {
			__db_rijndaelDecrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		__db_rijndaelDecrypt(key->rk, key->Nr, input, block);
		padLen = block[15];
		if (padLen >= 16) {
			return BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;

	case MODE_CBC:
		/* all blocks but last */
		memcpy(tmpiv, cipher->IV, MAX_IV_SIZE);
		for (i = numBlocks - 1; i > 0; i--) {
			__db_rijndaelDecrypt(key->rk, key->Nr, input, block);
			((u32*)block)[0] ^= tmpiv[0];
			((u32*)block)[1] ^= tmpiv[1];
			((u32*)block)[2] ^= tmpiv[2];
			((u32*)block)[3] ^= tmpiv[3];
			memcpy(tmpiv, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		__db_rijndaelDecrypt(key->rk, key->Nr, input, block);
		((u32*)block)[0] ^= tmpiv[0];
		((u32*)block)[1] ^= tmpiv[1];
		((u32*)block)[2] ^= tmpiv[2];
		((u32*)block)[3] ^= tmpiv[3];
		padLen = block[15];
		if (padLen <= 0 || padLen > 16) {
			return BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;

	default:
		return BAD_CIPHER_STATE;
	}

	return 16*numBlocks - padLen;
}

#ifdef INTERMEDIATE_VALUE_KAT
/**
 *	cipherUpdateRounds:
 *
 *	Encrypts/Decrypts exactly one full block a specified number of rounds.
 *	Only used in the Intermediate Value Known Answer Test.
 *
 *	Returns:
 *		TRUE - on success
 *		BAD_CIPHER_STATE - cipher in bad state (e.g., not initialized)
 */
/*
 * __db_cipherUpdateRounds --
 *
 * PUBLIC: int __db_cipherUpdateRounds __P((cipherInstance *, keyInstance *,
 * PUBLIC:    u_int8_t *, int, u_int8_t *, int));
 */
int
__db_cipherUpdateRounds(cipher, key, input, inputLen, outBuffer, rounds)
	cipherInstance *cipher;
	keyInstance *key;
	u_int8_t *input;
	size_t inputLen;
	u_int8_t *outBuffer;
	int rounds;
{
	u8 block[16];

	if (cipher == NULL || key == NULL) {
		return BAD_CIPHER_STATE;
	}

	memcpy(block, input, 16);

	switch (key->direction) {
	case DIR_ENCRYPT:
		__db_rijndaelEncryptRound(key->rk, key->Nr, block, rounds);
		break;

	case DIR_DECRYPT:
		__db_rijndaelDecryptRound(key->rk, key->Nr, block, rounds);
		break;

	default:
		return BAD_KEY_DIR;
	}

	memcpy(outBuffer, block, 16);

	return TRUE;
}
#endif /* INTERMEDIATE_VALUE_KAT */
