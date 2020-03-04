// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/proof.h>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";

namespace proof
{

Result SignMessage::Prepare(const std::string& message, std::set<CScript>& inputs_out, uint256& sighash_out, CScript& spk_out) const {
    Result rv = Purpose::Prepare(m_scriptpubkey, inputs_out);
    if (rv != Result::Valid) return rv;
    CHashWriter hw(SER_DISK, 0);
    std::string s = MESSAGE_MAGIC + message;
    hw << m_scriptpubkey << s;
    sighash_out = hw.GetHash();
    spk_out = m_scriptpubkey;
    return Result::Valid;
}

void SignMessageWithSigningProvider(std::unique_ptr<SigningProvider> sp, const std::string& message, const CTxDestination& destination, std::vector<uint8_t>& signature_out)
{
    signature_out.clear();

    // if this is a P2PKH, use the legacy approach
    const PKHash *pkhash = boost::get<PKHash>(&destination);
    if (pkhash) {
        CKey key;
        if (!sp->GetKey(CKeyID(*pkhash), key)) {
            throw privkey_unavailable_error();
        }

        CHashWriter ss(SER_GETHASH, 0);
        ss << MESSAGE_MAGIC << message;

        if (!key.SignCompact(ss.GetHash(), signature_out)) {
            throw signing_error();
        }
    } else {
        SignMessageWorkspace p;

        p.AppendDestinationChallenge(destination);

        p.Prove(message, std::move(sp));

        CVectorWriter w(SER_DISK, PROTOCOL_VERSION, signature_out, 0);
        w << p.m_proof;
    }
}

void SignMessageWithPrivateKey(const CKey& key, OutputType address_type, const std::string& message, std::vector<uint8_t>& signature_out)
{
    if (address_type == OutputType::LEGACY) {
        CHashWriter ss(SER_GETHASH, 0);
        ss << MESSAGE_MAGIC << message;

        if (!key.SignCompact(ss.GetHash(), signature_out)) {
            throw signing_error();
        }
    } else {
        SignMessageWorkspace p;

        p.AppendPrivKeyChallenge(key, address_type);

        p.Prove(message);

        CVectorWriter w(SER_DISK, PROTOCOL_VERSION, signature_out, 0);
        w << p.m_proof;
    }
}

Result VerifySignature(const std::string& message, const CTxDestination& destination, const std::vector<uint8_t>& signature)
{
    // if this is a P2PKH, use the legacy approach
    const PKHash* pkhash = boost::get<PKHash>(&destination);
    if (pkhash) {
        CHashWriter ss(SER_GETHASH, 0);
        ss << MESSAGE_MAGIC << message;
        CPubKey pubkey;
        return ResultFromBool(pubkey.RecoverCompact(ss.GetHash(), signature) && pubkey.GetID() == *pkhash);
    }

    SignMessageWorkspace p;

    p.AppendDestinationChallenge(destination);

    CDataStream stream(signature, SER_DISK, PROTOCOL_VERSION);
    try {
        stream >> p.m_proof;
        return p.Verify(message);
    } catch (const std::runtime_error&) {
        return Result::Error;
    }
}

} // namespace proof
