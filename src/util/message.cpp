// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>            // For CHashWriter
#include <key.h>             // For CKey
#include <key_io.h>          // For DecodeDestination()
#include <pubkey.h>          // For CPubKey
#include <script/standard.h> // For CTxDestination, IsValidDestination(), PKHash
#include <signet.h>          // For BIP-0322
#include <streams.h>         // For VectorReader
#include <serialize.h>       // For SER_GETHASH
#include <util/message.h>
#include <util/strencodings.h> // For DecodeBase64()

#include <string>
#include <vector>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";

MessageVerificationResult MessageVerifyBIP0322(
    const CTxDestination& destination,
    const std::vector<unsigned char>& signature,
    const std::string& message,
    const std::vector<COutPoint>& inputs,
    MessageVerificationResult inbound_result = MessageVerificationResult::ERR_ADDRESS_NO_KEY)
{
    bool forward_inbound = inbound_result != MessageVerificationResult::ERR_ADDRESS_NO_KEY;

    CMutableTransaction to_spend, to_sign;
    VectorReader v(SER_NETWORK, INIT_PROTO_VERSION, signature, 0);
    v >> to_spend >> to_sign;
    // No extraneous data after stream
    if (!v.empty()) return MessageVerificationResult::ERR_INVALID;
    auto challenge = GetScriptForDestination(destination);
    if (challenge.empty()) challenge = CScript(OP_TRUE);

    TransactionProofResult res = CheckTransactionProof(message, challenge, CTransaction(to_spend), CTransaction(to_sign));
    switch (res) {
    case TransactionProofInvalid: return forward_inbound ? inbound_result : MessageVerificationResult::ERR_INVALID;
    case TransactionProofInconclusive: return MessageVerificationResult::INCONCLUSIVE;
    case TransactionProofInconclusive | TransactionProofInFutureFlag: return MessageVerificationResult::INCONCLUSIVE_IN_FUTURE;
    case TransactionProofValid | TransactionProofInFutureFlag: return MessageVerificationResult::VALID_IN_FUTURE;
    }
    return MessageVerificationResult::OK;
}

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message,
    const std::vector<COutPoint>& inputs)
{
    bool invalid = false;
    std::vector<unsigned char> signature_bytes = DecodeBase64(signature.c_str(), &invalid);
    if (invalid) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerifyBIP0322(destination, signature_bytes, message, inputs, MessageVerificationResult::ERR_INVALID_ADDRESS);
    }

    if (inputs.size() > 0) {
        return MessageVerifyBIP0322(destination, signature_bytes, message, inputs);
    }

    if (boost::get<PKHash>(&destination) == nullptr) {
        return MessageVerifyBIP0322(destination, signature_bytes, message, inputs);
    }

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(MessageHash(message), signature_bytes)) {
        return MessageVerifyBIP0322(destination, signature_bytes, message, inputs, MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED);
    }

    if (!(CTxDestination(PKHash(pubkey)) == destination)) {
        return MessageVerifyBIP0322(destination, signature_bytes, message, inputs, MessageVerificationResult::ERR_NOT_SIGNED);
    }

    return MessageVerificationResult::OK;
}

bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature)
{
    std::vector<unsigned char> signature_bytes;

    if (!privkey.SignCompact(MessageHash(message), signature_bytes)) {
        return false;
    }

    signature = EncodeBase64(signature_bytes);

    return true;
}

uint256 MessageHash(const std::string& message)
{
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << MESSAGE_MAGIC << message;

    return hasher.GetHash();
}

std::string SigningResultString(const SigningResult res)
{
    switch (res) {
        case SigningResult::OK:
            return "No error";
        case SigningResult::PRIVATE_KEY_NOT_AVAILABLE:
            return "Private key not available";
        case SigningResult::SIGNING_FAILED:
            return "Sign failed";
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}
