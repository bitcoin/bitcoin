#include <mw/crypto/PublicKeys.h>
#include "ConversionUtil.h"
#include "Context.h"

#include <mw/exceptions/CryptoException.h>
#include <mw/util/VectorUtil.h>

static Locked<Context> PUBKEY_CONTEXT(std::make_shared<Context>());

PublicKey PublicKeys::Calculate(const BigInt<32>& privateKey)
{
    const int verifyResult = secp256k1_ec_seckey_verify(PUBKEY_CONTEXT.Read()->Get(), privateKey.data());
    if (verifyResult != 1) {
        ThrowCrypto("Failed to verify secret key");
    }

    secp256k1_pubkey pubkey;
    const int createResult = secp256k1_ec_pubkey_create(
        PUBKEY_CONTEXT.Read()->Get(),
        &pubkey,
        privateKey.data()
    );
    if (createResult != 1) {
        ThrowCrypto("Failed to calculate public key");
    }

    return ConversionUtil::ToPublicKey(pubkey);
}

PublicKey PublicKeys::Convert(const Commitment& commitment)
{
    return ConversionUtil::ToPublicKey(commitment);
}

PublicKey PublicKeys::Add(const std::vector<PublicKey>& publicKeys, const std::vector<PublicKey>& subtract)
{
    std::vector<secp256k1_pubkey> pubkeys = ConversionUtil::ToSecp256k1(publicKeys);
    std::vector<secp256k1_pubkey*> pubkeyPtrs = VectorUtil::ToPointerVec(pubkeys);

    std::vector<secp256k1_pubkey> to_negate = ConversionUtil::ToSecp256k1(subtract);
    std::transform(
        to_negate.begin(), to_negate.end(), std::back_inserter(pubkeyPtrs),
        [](secp256k1_pubkey& pubkey) {
            const int negate_status = secp256k1_ec_pubkey_negate(PUBKEY_CONTEXT.Read()->Get(), &pubkey);
            if (negate_status != 1) {
                ThrowCrypto("Failed to negate public key.");
            }

            return &pubkey;
        }
    );

    secp256k1_pubkey pubkey;
    const int pubKeysCombined = secp256k1_ec_pubkey_combine(
        PUBKEY_CONTEXT.Read()->Get(),
        &pubkey,
        pubkeyPtrs.data(),
        pubkeyPtrs.size()
    );
    if (pubKeysCombined != 1) {
        ThrowCrypto("Failed to combine public keys.");
    }

    return ConversionUtil::ToPublicKey(pubkey);
}

PublicKey PublicKeys::MultiplyKey(const PublicKey& public_key, const SecretKey& mul)
{
    secp256k1_pubkey pubkey = ConversionUtil::ToSecp256k1(public_key);
    const int tweakResult = secp256k1_ec_pubkey_tweak_mul(
        PUBKEY_CONTEXT.Read()->Get(),
        &pubkey,
        mul.data()
    );
    if (tweakResult != 1) {
        ThrowCrypto("secp256k1_ec_pubkey_tweak_mul failed");
    }

    return ConversionUtil::ToPublicKey(pubkey);
}

PublicKey PublicKeys::DivideKey(const PublicKey& public_key, const SecretKey& div)
{
    SecretKey inv = div;
    const int inv_result = secp256k1_ec_privkey_tweak_inv(PUBKEY_CONTEXT.Read()->Get(), inv.data());
    if (inv_result != 1) {
        ThrowCrypto("secp256k1_ec_privkey_tweak_inv failed");
    }

    secp256k1_pubkey pubkey = ConversionUtil::ToSecp256k1(public_key);
    const int mul_result = secp256k1_ec_pubkey_tweak_mul(
        PUBKEY_CONTEXT.Read()->Get(),
        &pubkey,
        inv.data()
    );
    if (mul_result != 1) {
        ThrowCrypto("secp256k1_ec_pubkey_tweak_mul failed");
    }

    return ConversionUtil::ToPublicKey(pubkey);
}