// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/crypter.h>

namespace wallet {
namespace {

const TestingSetup* g_setup;
void initialize_crypter()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(crypter, .init = initialize_crypter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};

    CCrypter crypt;
    // These values are regularly updated within `CallOneOf`
    std::vector<unsigned char> cipher_text_ed;
    CKeyingMaterial plain_text_ed;
    const std::vector<unsigned char> random_key = ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_KEY_SIZE);

    if (fuzzed_data_provider.ConsumeBool()) {
        const std::string random_string = fuzzed_data_provider.ConsumeRandomLengthString(100);
        SecureString secure_string(random_string.begin(), random_string.end());

        const unsigned int derivation_method = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<unsigned int>();

        // Limiting the value of rounds since it is otherwise uselessly expensive and causes a timeout when fuzzing.
        crypt.SetKeyFromPassphrase(/*key_data=*/secure_string,
                                   /*salt=*/ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_SALT_SIZE),
                                   /*rounds=*/fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 25000),
                                   /*derivation_method=*/derivation_method);
    }

    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 100)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> random_vector = ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_KEY_SIZE);
                plain_text_ed = CKeyingMaterial(random_vector.begin(), random_vector.end());
            },
            [&] {
                cipher_text_ed = ConsumeRandomLengthByteVector(fuzzed_data_provider, 64);
            },
            [&] {
                (void)crypt.Encrypt(plain_text_ed, cipher_text_ed);
            },
            [&] {
                (void)crypt.Decrypt(cipher_text_ed, plain_text_ed);
            },
            [&] {
                const CKeyingMaterial master_key(random_key.begin(), random_key.end());
                const uint256 iv = ConsumeUInt256(fuzzed_data_provider);
                (void)EncryptSecret(master_key, plain_text_ed, iv, cipher_text_ed);
            },
            [&] {
                const CKeyingMaterial master_key(random_key.begin(), random_key.end());
                const uint256 iv = ConsumeUInt256(fuzzed_data_provider);
                (void)DecryptSecret(master_key, cipher_text_ed, iv, plain_text_ed);
            },
            [&] {
                std::optional<CPubKey> random_pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
                if (!random_pub_key) {
                    good_data = false;
                    return;
                }
                const CPubKey pub_key = *random_pub_key;
                const CKeyingMaterial master_key(random_key.begin(), random_key.end());
                const std::vector<unsigned char> crypted_secret = ConsumeRandomLengthByteVector(fuzzed_data_provider, 64);
                CKey key;
                (void)DecryptKey(master_key, crypted_secret, pub_key, key);
            });
    }
}
} // namespace
} // namespace wallet
