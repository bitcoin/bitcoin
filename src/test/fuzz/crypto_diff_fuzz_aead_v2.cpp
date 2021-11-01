/**
 * Alternate ChaCha20Forward4064-Poly1305@Bitcoin cipher suite construction
 */

struct AltChaCha20Forward4064 {
    ChaCha20 chacha;
    int iv = 0;
    int keystream_pos = 0;
    unsigned char keystream[4096] = {0};
};

struct AltChaCha20Forward4064Poly1305 {
    struct AltChaCha20Forward4064 F;
    struct AltChaCha20Forward4064 V;
};

void initialise(AltChaCha20Forward4064& instance)
{
    instance.chacha.SetIV(instance.iv);
    instance.chacha.Keystream(instance.keystream, 4096);
}

/**
 * Rekey when keystream_pos=4064
 */
void rekey(AltChaCha20Forward4064& instance)
{
    if (instance.keystream_pos == 4064) {
        instance.chacha.SetKey(instance.keystream + 4064, 32);
        instance.chacha.SetIV(++instance.iv);
        instance.chacha.Keystream(instance.keystream, 4096);
        instance.keystream_pos = 0;
    }
}

/**
 * Encrypting a message
 * Input:  Message m and Payload length bytes
 * Output: Ciphertext c consists of: (1)+(2)+(3)
 */
bool Encryption(const unsigned char* m, unsigned char* c, int bytes, AltChaCha20Forward4064& F, AltChaCha20Forward4064& V)
{
    if (bytes < 0)
        return false;

    int ptr = 0;
    // (1) 3 bytes LE of message m
    for (int i = 0; i < 3; ++i) {
        c[ptr] = F.keystream[F.keystream_pos] ^ m[i];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
        ++ptr;
    }

    // (2) encrypted message m
    for (int i = 0; i < bytes; ++i) {
        c[ptr] = V.keystream[V.keystream_pos] ^ m[i + 3];
        ++V.keystream_pos;
        if (V.keystream_pos == 4064) {
            rekey(V);
        }
        ++ptr;
    }

    // (3) 16 byte MAC tag
    unsigned char key[POLY1305_KEYLEN];
    for (int i = 0; i < POLY1305_KEYLEN; ++i) {
        key[i] = F.keystream[F.keystream_pos];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    poly1305_auth(c + bytes + 3, c, bytes + 3, key);
    memory_cleanse(key, POLY1305_KEYLEN);

    return true;
}

/**
 * Decrypting the 3 bytes Payload length
 */
int DecryptionAAD(const unsigned char* c, AltChaCha20Forward4064& F)
{
    uint32_t x;
    unsigned char length[3];
    for (int i = 0; i < 3; ++i) {
        length[i] = F.keystream[F.keystream_pos] ^ c[i];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    memcpy((char*)&x, length, 3);
    return le32toh(x);
}

/**
 * Decrypting a message
 * Input:  Ciphertext c consists of: (1)+(2)+(3) and Payload length bytes
 * Output: Message m
 */
bool Decryption(unsigned char* m, const unsigned char* c, int bytes, AltChaCha20Forward4064& F, AltChaCha20Forward4064& V)
{
    if (bytes < 0)
        return false;

    /// (1) Decrypt first 3 bytes from c as LE of message m. This is done before calling Decryption().
    /// It's necessary since F's keystream is required for decryption of length
    /// and can get lost if rekeying of F happens during poly1305 key generation.
    for (int i = 0; i < 3; ++i) {
        m[i] = c[i];
    }

    // (3) 16 byte MAC tag
    unsigned char out[POLY1305_TAGLEN];
    unsigned char key[POLY1305_KEYLEN];
    for (int i = 0; i < POLY1305_KEYLEN; ++i) {
        key[i] = F.keystream[F.keystream_pos];
        ++F.keystream_pos;
        if (F.keystream_pos == 4064) {
            rekey(F);
        }
    }
    poly1305_auth(out, c, bytes + 3, key);
    if (memcmp(out, c + 3 + bytes, POLY1305_TAGLEN) != 0) return false;
    memory_cleanse(key, POLY1305_KEYLEN);
    memory_cleanse(out, POLY1305_TAGLEN);

    // (2) decrypted message m
    for (int i = 0; i < bytes; ++i) {
        m[i + 3] = V.keystream[V.keystream_pos] ^ c[i + 3];
        ++V.keystream_pos;
        if (V.keystream_pos == 4064) {
            rekey(V);
        }
    }
    return true;
}