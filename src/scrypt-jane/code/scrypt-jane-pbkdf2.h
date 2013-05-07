typedef struct scrypt_hmac_state_t {
	scrypt_hash_state inner, outer;
} scrypt_hmac_state;


static void
scrypt_hash(scrypt_hash_digest hash, const uint8_t *m, size_t mlen) {
	scrypt_hash_state st;
	scrypt_hash_init(&st);
	scrypt_hash_update(&st, m, mlen);
	scrypt_hash_finish(&st, hash);
}

/* hmac */
static void
scrypt_hmac_init(scrypt_hmac_state *st, const uint8_t *key, size_t keylen) {
	uint8_t pad[SCRYPT_HASH_BLOCK_SIZE] = {0};
	size_t i;

	scrypt_hash_init(&st->inner);
	scrypt_hash_init(&st->outer);

	if (keylen <= SCRYPT_HASH_BLOCK_SIZE) {
		/* use the key directly if it's <= blocksize bytes */
		memcpy(pad, key, keylen);
	} else {
		/* if it's > blocksize bytes, hash it */
		scrypt_hash(pad, key, keylen);
	}

	/* inner = (key ^ 0x36) */
	/* h(inner || ...) */
	for (i = 0; i < SCRYPT_HASH_BLOCK_SIZE; i++)
		pad[i] ^= 0x36;
	scrypt_hash_update(&st->inner, pad, SCRYPT_HASH_BLOCK_SIZE);

	/* outer = (key ^ 0x5c) */
	/* h(outer || ...) */
	for (i = 0; i < SCRYPT_HASH_BLOCK_SIZE; i++)
		pad[i] ^= (0x5c ^ 0x36);
	scrypt_hash_update(&st->outer, pad, SCRYPT_HASH_BLOCK_SIZE);

	scrypt_ensure_zero(pad, sizeof(pad));
}

static void
scrypt_hmac_update(scrypt_hmac_state *st, const uint8_t *m, size_t mlen) {
	/* h(inner || m...) */
	scrypt_hash_update(&st->inner, m, mlen);
}

static void
scrypt_hmac_finish(scrypt_hmac_state *st, scrypt_hash_digest mac) {
	/* h(inner || m) */
	scrypt_hash_digest innerhash;
	scrypt_hash_finish(&st->inner, innerhash);

	/* h(outer || h(inner || m)) */
	scrypt_hash_update(&st->outer, innerhash, sizeof(innerhash));
	scrypt_hash_finish(&st->outer, mac);

	scrypt_ensure_zero(st, sizeof(*st));
}

static void
scrypt_pbkdf2(const uint8_t *password, size_t password_len, const uint8_t *salt, size_t salt_len, uint64_t N, uint8_t *out, size_t bytes) {
	scrypt_hmac_state hmac_pw, hmac_pw_salt, work;
	scrypt_hash_digest ti, u;
	uint8_t be[4];
	uint32_t i, j, blocks;
	uint64_t c;
	
	/* bytes must be <= (0xffffffff - (SCRYPT_HASH_DIGEST_SIZE - 1)), which they will always be under scrypt */

	/* hmac(password, ...) */
	scrypt_hmac_init(&hmac_pw, password, password_len);

	/* hmac(password, salt...) */
	hmac_pw_salt = hmac_pw;
	scrypt_hmac_update(&hmac_pw_salt, salt, salt_len);

	blocks = ((uint32_t)bytes + (SCRYPT_HASH_DIGEST_SIZE - 1)) / SCRYPT_HASH_DIGEST_SIZE;
	for (i = 1; i <= blocks; i++) {
		/* U1 = hmac(password, salt || be(i)) */
		U32TO8_BE(be, i);
		work = hmac_pw_salt;
		scrypt_hmac_update(&work, be, 4);
		scrypt_hmac_finish(&work, ti);
		memcpy(u, ti, sizeof(u));

		/* T[i] = U1 ^ U2 ^ U3... */
		for (c = 0; c < N - 1; c++) {
			/* UX = hmac(password, U{X-1}) */
			work = hmac_pw;
			scrypt_hmac_update(&work, u, SCRYPT_HASH_DIGEST_SIZE);
			scrypt_hmac_finish(&work, u);

			/* T[i] ^= UX */
			for (j = 0; j < sizeof(u); j++)
				ti[j] ^= u[j];
		}

		memcpy(out, ti, (bytes > SCRYPT_HASH_DIGEST_SIZE) ? SCRYPT_HASH_DIGEST_SIZE : bytes);
		out += SCRYPT_HASH_DIGEST_SIZE;
		bytes -= SCRYPT_HASH_DIGEST_SIZE;
	}

	scrypt_ensure_zero(ti, sizeof(ti));
	scrypt_ensure_zero(u, sizeof(u));
	scrypt_ensure_zero(&hmac_pw, sizeof(hmac_pw));
	scrypt_ensure_zero(&hmac_pw_salt, sizeof(hmac_pw_salt));
}
