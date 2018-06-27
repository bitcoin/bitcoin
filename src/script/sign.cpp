// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/sign.h>

#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <uint256.h>


typedef std::vector<unsigned char> valtype;

MutableTransactionSignatureCreator::MutableTransactionSignatureCreator(const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) : txTo(txToIn), nIn(nInIn), nHashType(nHashTypeIn), amount(amountIn), checker(txTo, nIn, amountIn) {}

bool MutableTransactionSignatureCreator::CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
{
    CKey key;
    if (!provider.GetKey(address, key))
        return false;

    // Signing with uncompressed keys is disabled in witness scripts
    if (sigversion == SigVersion::WITNESS_V0 && !key.IsCompressed())
        return false;

    uint256 hash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, sigversion);
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)nHashType);
    return true;
}

static bool GetCScript(const SigningProvider& provider, const SignatureData& sigdata, const CScriptID& scriptid, CScript& script)
{
    if (provider.GetCScript(scriptid, script)) {
        return true;
    }
    // Look for scripts in SignatureData
    if (CScriptID(sigdata.redeem_script) == scriptid) {
        script = sigdata.redeem_script;
        return true;
    } else if (CScriptID(sigdata.witness_script) == scriptid) {
        script = sigdata.witness_script;
        return true;
    }
    return false;
}

static bool GetPubKey(const SigningProvider& provider, const SignatureData& sigdata, const CKeyID& address, CPubKey& pubkey)
{
    if (provider.GetPubKey(address, pubkey)) {
        return true;
    }
    // Look for pubkey in all partial sigs
    const auto it = sigdata.signatures.find(address);
    if (it != sigdata.signatures.end()) {
        pubkey = it->second.first;
        return true;
    }
    return false;
}

static bool CreateSig(const BaseSignatureCreator& creator, SignatureData& sigdata, const SigningProvider& provider, std::vector<unsigned char>& sig_out, const CKeyID& keyid, const CScript& scriptcode, SigVersion sigversion)
{
    const auto it = sigdata.signatures.find(keyid);
    if (it != sigdata.signatures.end()) {
        sig_out = it->second.second;
        return true;
    }
    if (creator.CreateSig(provider, sig_out, keyid, scriptcode, sigversion)) {
        CPubKey pubkey;
        GetPubKey(provider, sigdata, keyid, pubkey);
        auto i = sigdata.signatures.emplace(keyid, SigPair(pubkey, sig_out));
        assert(i.second);
        return true;
    }
    return false;
}

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TX_SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
static bool SignStep(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                     std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion, SignatureData& sigdata)
{
    CScript scriptRet;
    uint160 h160;
    ret.clear();
    std::vector<unsigned char> sig;

    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
        return false;

    switch (whichTypeRet)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_UNKNOWN:
        return false;
    case TX_PUBKEY:
        if (!CreateSig(creator, sigdata, provider, sig, CPubKey(vSolutions[0]).GetID(), scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        return true;
    case TX_PUBKEYHASH: {
        CKeyID keyID = CKeyID(uint160(vSolutions[0]));
        if (!CreateSig(creator, sigdata, provider, sig, keyID, scriptPubKey, sigversion)) return false;
        ret.push_back(std::move(sig));
        CPubKey pubkey;
        GetPubKey(provider, sigdata, keyID, pubkey);
        ret.push_back(ToByteVector(pubkey));
        return true;
    }
    case TX_SCRIPTHASH:
        if (GetCScript(provider, sigdata, uint160(vSolutions[0]), scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
        }
        return false;

    case TX_MULTISIG: {
        size_t required = vSolutions.front()[0];
        ret.push_back(valtype()); // workaround CHECKMULTISIG bug
        for (size_t i = 1; i < vSolutions.size() - 1; ++i) {
            CPubKey pubkey = CPubKey(vSolutions[i]);
            if (ret.size() < required + 1 && CreateSig(creator, sigdata, provider, sig, pubkey.GetID(), scriptPubKey, sigversion)) {
                ret.push_back(std::move(sig));
            }
        }
        bool ok = ret.size() == required + 1;
        for (size_t i = 0; i + ret.size() < required + 1; ++i) {
            ret.push_back(valtype());
        }
        return ok;
    }
    case TX_WITNESS_V0_KEYHASH:
        ret.push_back(vSolutions[0]);
        return true;

    case TX_WITNESS_V0_SCRIPTHASH:
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160.begin());
        if (GetCScript(provider, sigdata, h160, scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
        }
        return false;

    default:
        return false;
    }
}

static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else {
            result << v;
        }
    }
    return result;
}

bool ProduceSignature(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata)
{
    if (sigdata.complete) return true;

    std::vector<valtype> result;
    txnouttype whichType;
    bool solved = SignStep(provider, creator, fromPubKey, result, whichType, SigVersion::BASE, sigdata);
    bool P2SH = false;
    CScript subscript;
    sigdata.scriptWitness.stack.clear();

    if (solved && whichType == TX_SCRIPTHASH)
    {
        // Solver returns the subscript that needs to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        subscript = CScript(result[0].begin(), result[0].end());
        sigdata.redeem_script = subscript;
        solved = solved && SignStep(provider, creator, subscript, result, whichType, SigVersion::BASE, sigdata) && whichType != TX_SCRIPTHASH;
        P2SH = true;
    }

    if (solved && whichType == TX_WITNESS_V0_KEYHASH)
    {
        CScript witnessscript;
        witnessscript << OP_DUP << OP_HASH160 << ToByteVector(result[0]) << OP_EQUALVERIFY << OP_CHECKSIG;
        txnouttype subType;
        solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata);
        sigdata.scriptWitness.stack = result;
        result.clear();
    }
    else if (solved && whichType == TX_WITNESS_V0_SCRIPTHASH)
    {
        CScript witnessscript(result[0].begin(), result[0].end());
        sigdata.witness_script = witnessscript;
        txnouttype subType;
        solved = solved && SignStep(provider, creator, witnessscript, result, subType, SigVersion::WITNESS_V0, sigdata) && subType != TX_SCRIPTHASH && subType != TX_WITNESS_V0_SCRIPTHASH && subType != TX_WITNESS_V0_KEYHASH;
        result.push_back(std::vector<unsigned char>(witnessscript.begin(), witnessscript.end()));
        sigdata.scriptWitness.stack = result;
        result.clear();
    }

    if (P2SH) {
        result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
    }
    sigdata.scriptSig = PushAll(result);

    // Test solution
    sigdata.complete = solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
    return sigdata.complete;
}

class SignatureExtractorChecker final : public BaseSignatureChecker
{
private:
    SignatureData& sigdata;
    BaseSignatureChecker& checker;

public:
    SignatureExtractorChecker(SignatureData& sigdata, BaseSignatureChecker& checker) : sigdata(sigdata), checker(checker) {}
    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
};

bool SignatureExtractorChecker::CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
{
    if (checker.CheckSig(scriptSig, vchPubKey, scriptCode, sigversion)) {
        CPubKey pubkey(vchPubKey);
        sigdata.signatures.emplace(pubkey.GetID(), SigPair(pubkey, scriptSig));
        return true;
    }
    return false;
}

namespace
{
struct Stacks
{
    std::vector<valtype> script;
    std::vector<valtype> witness;

    Stacks() {}
    explicit Stacks(const std::vector<valtype>& scriptSigStack_) : script(scriptSigStack_), witness() {}
    explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
        EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
    }

    SignatureData Output() const {
        SignatureData result;
        result.scriptSig = PushAll(script);
        result.scriptWitness.stack = witness;
        return result;
    }
};
}

// Extracts signatures and scripts from incomplete scriptSigs. Please do not extend this, use PSBT instead
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout)
{
    SignatureData data;
    assert(tx.vin.size() > nIn);
    data.scriptSig = tx.vin[nIn].scriptSig;
    data.scriptWitness = tx.vin[nIn].scriptWitness;
    Stacks stack(data);

    // Get signatures
    MutableTransactionSignatureChecker tx_checker(&tx, nIn, txout.nValue);
    SignatureExtractorChecker extractor_checker(data, tx_checker);
    if (VerifyScript(data.scriptSig, txout.scriptPubKey, &data.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, extractor_checker)) {
        data.complete = true;
        return data;
    }

    // Get scripts
    txnouttype script_type;
    std::vector<std::vector<unsigned char>> solutions;
    Solver(txout.scriptPubKey, script_type, solutions);
    SigVersion sigversion = SigVersion::BASE;
    CScript next_script = txout.scriptPubKey;

    if (script_type == TX_SCRIPTHASH && !stack.script.empty() && !stack.script.back().empty()) {
        // Get the redeemScript
        CScript redeem_script(stack.script.back().begin(), stack.script.back().end());
        data.redeem_script = redeem_script;
        next_script = std::move(redeem_script);

        // Get redeemScript type
        Solver(next_script, script_type, solutions);
        stack.script.pop_back();
    }
    if (script_type == TX_WITNESS_V0_SCRIPTHASH && !stack.witness.empty() && !stack.witness.back().empty()) {
        // Get the witnessScript
        CScript witness_script(stack.witness.back().begin(), stack.witness.back().end());
        data.witness_script = witness_script;
        next_script = std::move(witness_script);

        // Get witnessScript type
        Solver(next_script, script_type, solutions);
        stack.witness.pop_back();
        stack.script = std::move(stack.witness);
        stack.witness.clear();
        sigversion = SigVersion::WITNESS_V0;
    }
    if (script_type == TX_MULTISIG && !stack.script.empty()) {
        // Build a map of pubkey -> signature by matching sigs to pubkeys:
        assert(solutions.size() > 1);
        unsigned int num_pubkeys = solutions.size()-2;
        unsigned int last_success_key = 0;
        for (const valtype& sig : stack.script) {
            for (unsigned int i = last_success_key; i < num_pubkeys; ++i) {
                const valtype& pubkey = solutions[i+1];
                // We either have a signature for this pubkey, or we have found a signature and it is valid
                if (data.signatures.count(CPubKey(pubkey).GetID()) || extractor_checker.CheckSig(sig, pubkey, next_script, sigversion)) {
                    last_success_key = i + 1;
                    break;
                }
            }
        }
    }

    return data;
}

void UpdateInput(CTxIn& input, const SignatureData& data)
{
    input.scriptSig = data.scriptSig;
    input.scriptWitness = data.scriptWitness;
}

void SignatureData::MergeSignatureData(SignatureData sigdata)
{
    if (complete) return;
    if (sigdata.complete) {
        *this = std::move(sigdata);
        return;
    }
    if (redeem_script.empty() && !sigdata.redeem_script.empty()) {
        redeem_script = sigdata.redeem_script;
    }
    if (witness_script.empty() && !sigdata.witness_script.empty()) {
        witness_script = sigdata.witness_script;
    }
    signatures.insert(std::make_move_iterator(sigdata.signatures.begin()), std::make_move_iterator(sigdata.signatures.end()));
}

bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType)
{
    assert(nIn < txTo.vin.size());

    MutableTransactionSignatureCreator creator(&txTo, nIn, amount, nHashType);

    SignatureData sigdata;
    bool ret = ProduceSignature(provider, creator, fromPubKey, sigdata);
    UpdateInput(txTo.vin.at(nIn), sigdata);
    return ret;
}

bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignature(provider, txout.scriptPubKey, txTo, nIn, txout.nValue, nHashType);
}

namespace {
/** Dummy signature checker which accepts all signatures. */
class DummySignatureChecker final : public BaseSignatureChecker
{
public:
    DummySignatureChecker() {}
    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override { return true; }
};
const DummySignatureChecker DUMMY_CHECKER;

class DummySignatureCreator final : public BaseSignatureCreator {
public:
    DummySignatureCreator() {}
    const BaseSignatureChecker& Checker() const override { return DUMMY_CHECKER; }
    bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override
    {
        // Create a dummy signature that is a valid DER-encoding
        vchSig.assign(72, '\000');
        vchSig[0] = 0x30;
        vchSig[1] = 69;
        vchSig[2] = 0x02;
        vchSig[3] = 33;
        vchSig[4] = 0x01;
        vchSig[4 + 33] = 0x02;
        vchSig[5 + 33] = 32;
        vchSig[6 + 33] = 0x01;
        vchSig[6 + 33 + 32] = SIGHASH_ALL;
        return true;
    }
};
}

const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR = DummySignatureCreator();
const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();

bool IsSolvable(const SigningProvider& provider, const CScript& script)
{
    // This check is to make sure that the script we created can actually be solved for and signed by us
    // if we were to have the private keys. This is just to make sure that the script is valid and that,
    // if found in a transaction, we would still accept and relay that transaction. In particular,
    // it will reject witness outputs that require signing with an uncompressed public key.
    SignatureData sigs;
    // Make sure that STANDARD_SCRIPT_VERIFY_FLAGS includes SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, the most
    // important property this function is designed to test for.
    static_assert(STANDARD_SCRIPT_VERIFY_FLAGS & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, "IsSolvable requires standard script flags to include WITNESS_PUBKEYTYPE");
    if (ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, script, sigs)) {
        // VerifyScript check is just defensive, and should never fail.
        assert(VerifyScript(sigs.scriptSig, script, &sigs.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER));
        return true;
    }
    return false;
}


bool PartiallySignedTransaction::IsNull() const
{
    return !tx && inputs.empty() && outputs.empty() && unknown.empty();
}

bool PSBTInput::IsNull() const
{
    return !non_witness_utxo && witness_utxo.IsNull() && partial_sigs.empty() && unknown.empty() && hd_keypaths.empty() && redeem_script.empty() && witness_script.empty();
}

bool PSBTOutput::IsNull() const
{
    return redeem_script.empty() && witness_script.empty() && hd_keypaths.empty() && unknown.empty();
}
