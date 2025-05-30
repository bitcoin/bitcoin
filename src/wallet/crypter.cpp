// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/crypter.h>

#include <common/system.h>
#include <crypto/aes.h>
#include <crypto/sha512.h>

#include <type_traits>
#include <vector>

namespace wallet {
int CCrypter::BytesToKeySHA512AES(const std::span<const unsigned char> salt, const SecureString& key_data, int count, unsigned char* key, unsigned char* iv) const
{
    // This mimics the behavior of openssl's EVP_BytesToKey with an aes256cbc
    // cipher and sha512 message digest. Because sha512's output size (64b) is
    // greater than the aes256 block size (16b) + aes256 key size (32b),
    // there's no need to process more than once (D_0).

    if(!count || !key || !iv)
        return 0;

    unsigned char buf[CSHA512::OUTPUT_SIZE];
    CSHA512 di;

    di.Write(UCharCast(key_data.data()), key_data.size());
    di.Write(salt.data(), salt.size());
    di.Finalize(buf);

    for(int i = 0; i != count - 1; i++)
        di.Reset().Write(buf, sizeof(buf)).Finalize(buf);

    memcpy(key, buf, WALLET_CRYPTO_KEY_SIZE);
    memcpy(iv, buf + WALLET_CRYPTO_KEY_SIZE, WALLET_CRYPTO_IV_SIZE);
    memory_cleanse(buf, sizeof(buf));
    return WALLET_CRYPTO_KEY_SIZE;
}

bool CCrypter::SetKeyFromPassphrase(const SecureString& key_data, const std::span<const unsigned char> salt, const unsigned int rounds, const unsigned int derivation_method)
{
    if (rounds < 1 || salt.size() != WALLET_CRYPTO_SALT_SIZE) {
        return false;
    }

    int i = 0;
    if (derivation_method == 0) {
        i = BytesToKeySHA512AES(salt, key_data, rounds, vchKey.data(), vchIV.data());
    }

    if (i != (int)WALLET_CRYPTO_KEY_SIZE)
    {
        memory_cleanse(vchKey.data(), vchKey.size());
        memory_cleanse(vchIV.data(), vchIV.size());
        return false;
    }

    fKeySet = true;
    return true;
}

bool CCrypter::SetKey(const CKeyingMaterial& new_key, const std::span<const unsigned char> new_iv)
{
    if (new_key.size() != WALLET_CRYPTO_KEY_SIZE || new_iv.size() != WALLET_CRYPTO_IV_SIZE) {
        return false;
    }

    memcpy(vchKey.data(), new_key.data(), new_key.size());
    memcpy(vchIV.data(), new_iv.data(), new_iv.size());

    fKeySet = true;
    return true;
}

bool CCrypter::Encrypt(const CKeyingMaterial& vchPlaintext, std::vector<unsigned char> &vchCiphertext) const
{
    if (!fKeySet)
        return false;

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCKSIZE bytes
    vchCiphertext.resize(vchPlaintext.size() + AES_BLOCKSIZE);

    AES256CBCEncrypt enc(vchKey.data(), vchIV.data(), true);
    size_t nLen = enc.Encrypt(vchPlaintext.data(), vchPlaintext.size(), vchCiphertext.data());
    if(nLen < vchPlaintext.size())
        return false;
    vchCiphertext.resize(nLen);

    return true;
}

bool CCrypter::Decrypt(const std::span<const unsigned char> ciphertext, CKeyingMaterial& plaintext) const
{
    if (!fKeySet)
        return false;

    // plaintext will always be equal to or lesser than length of ciphertext
    plaintext.resize(ciphertext.size());

    AES256CBCDecrypt dec(vchKey.data(), vchIV.data(), true);
    int len = dec.Decrypt(ciphertext.data(), ciphertext.size(), plaintext.data());
    if (len == 0) {
        return false;
    }
    plaintext.resize(len);
    return true;
}

bool EncryptSecret(const CKeyingMaterial& vMasterKey, const CKeyingMaterial &vchPlaintext, const uint256& nIV, std::vector<unsigned char> &vchCiphertext)
{
    CCrypter cKeyCrypter;
    std::vector<unsigned char> chIV(WALLET_CRYPTO_IV_SIZE);
    memcpy(chIV.data(), &nIV, WALLET_CRYPTO_IV_SIZE);
    if(!cKeyCrypter.SetKey(vMasterKey, chIV))
        return false;
    return cKeyCrypter.Encrypt(vchPlaintext, vchCiphertext);
}

bool DecryptSecret(const CKeyingMaterial& master_key, const std::span<const unsigned char> ciphertext, const uint256& iv, CKeyingMaterial& plaintext)
{
    CCrypter key_crypter;
    static_assert(WALLET_CRYPTO_IV_SIZE <= std::remove_reference_t<decltype(iv)>::size());
    const std::span iv_prefix{iv.data(), WALLET_CRYPTO_IV_SIZE};
    if (!key_crypter.SetKey(master_key, iv_prefix)) {
        return false;
    }
    return key_crypter.Decrypt(ciphertext, plaintext);
}

bool DecryptKey(const CKeyingMaterial& master_key, const std::span<const unsigned char> crypted_secret, const CPubKey& pub_key, CKey& key)
{
    CKeyingMaterial secret;
    if (!DecryptSecret(master_key, crypted_secret, pub_key.GetHash(), secret)) {
        return false;
    }

    if (secret.size() != 32) {
        return false;
    }

    key.Set(secret.begin(), secret.end(), pub_key.IsCompressed());
    return key.VerifyPubKey(pub_key);
}
} // namespace wallet
