#include <silentpayment.h>

#include <arith_uint256.h>
#include <coins.h>
#include <crypto/hmac_sha512.h>
#include <key_io.h>
#include <undo.h>
namespace silentpayment {

Recipient::Recipient(const CKey& spend_seckey, size_t pool_size)
{
    std::vector<std::pair<CKey, XOnlyPubKey>> spend_keys;

    unsigned char scan_seckey_bytes[32];
    CSHA256().Write(spend_seckey.begin(), 32).Finalize(scan_seckey_bytes);

    CKey scan_key;
    scan_key.Set(std::begin(scan_seckey_bytes), std::end(scan_seckey_bytes), true);

    m_scan_pubkey = XOnlyPubKey(scan_key.GetPubKey());

    m_negated_scan_seckey = scan_key;
    if (scan_key.GetPubKey().data()[0] == 3) {
        m_negated_scan_seckey.Negate();
    }

    for(size_t identifier = 0; identifier < pool_size; identifier++) {

        CKey spend_seckey1{spend_seckey};
        if (spend_seckey.GetPubKey().data()[0] == 3) {
            spend_seckey1.Negate();
        }

        arith_uint256 tweak;
        tweak = tweak + identifier;

        CKey spend_seckey2 = spend_seckey1.AddTweak(ArithToUint256(tweak).data());

        CKey tweaked_spend_seckey{spend_seckey2};
        if (spend_seckey2.GetPubKey().data()[0] == 3) {
            tweaked_spend_seckey.Negate();
        }

        spend_keys.push_back(std::make_pair(tweaked_spend_seckey, XOnlyPubKey{tweaked_spend_seckey.GetPubKey()}));
    }

    m_spend_keys = spend_keys;
}

void Recipient::SetSenderPublicKey(const CPubKey& sender_public_key, const uint256& outpoint_hash)
{
    auto tweaked_scan_seckey = m_negated_scan_seckey.MultiplyTweak(outpoint_hash.begin());

    std::array<unsigned char, 32> result = tweaked_scan_seckey.ECDH(sender_public_key);

    std::copy(std::begin(result), std::end(result), std::begin(m_shared_secret));
}

void Recipient::SetSenderPublicKey(const CPubKey& sender_public_key, const std::vector<COutPoint>& tx_outpoints)
{
    const auto& [truncated_hash, outpoint_hash] = HashOutpoints(tx_outpoints);
    (void) truncated_hash; //not used here

    SetSenderPublicKey(sender_public_key, outpoint_hash);
}

std::tuple<CKey,XOnlyPubKey> Recipient::Tweak(const int32_t& identifier) const
{
    const auto& [seckey, xonly_pubkey]{m_spend_keys.at(identifier)};

    const auto& result_xonly_pubkey{xonly_pubkey.AddTweak(m_shared_secret)};

    const auto& result_seckey{seckey.AddTweak(m_shared_secret)};

    return {result_seckey, result_xonly_pubkey};
}

std::pair<XOnlyPubKey,XOnlyPubKey> Recipient::GetAddress(const int32_t& identifier) const
{
    const auto& [_, spend_pubkey]{m_spend_keys.at(identifier)};
    return {m_scan_pubkey, spend_pubkey};
}

int32_t Recipient::GetIdentifier(const XOnlyPubKey& spend_pubkey) const
{
    for(std::size_t i = 0; i < spend_pubkey.size(); i++) {
        const auto& [_, pubkey] = m_spend_keys.at(i);
        if (pubkey == spend_pubkey) {
            return i;
        }
    }
    return -1;
}

CPubKey Recipient::CombinePublicKeys(const std::vector<CPubKey>& sender_public_keys, const std::vector<XOnlyPubKey>& sender_x_only_public_key)
{
    std::vector<CPubKey> v_pubkeys;

    v_pubkeys.insert(v_pubkeys.end(), sender_public_keys.begin(), sender_public_keys.end());

    for (auto& xpubkey : sender_x_only_public_key) {
        v_pubkeys.push_back(xpubkey.ConvertToCompressedPubKey());
    }

    return CPubKey::Combine(v_pubkeys);
}

CPubKey Recipient::CombinePublicKeys(const CTransaction &tx, const std::vector<Coin>& coins)
{
    CPubKey sum_sender_pubkeys;

    std::vector<XOnlyPubKey> input_xonly_pubkeys;
    std::vector<CPubKey> input_pubkeys;

    for (size_t i = 0; i < tx.vin.size(); i++)
    {
        const CTxIn& txin{tx.vin.at(i)};
        const Coin& prev_coin{coins.at(i)};

        // TODO: confirm that txin.scriptSig unlocks prev_coin.out.scriptPubKey

        const auto& pubkey_variant{ExtractPubkeyFromInput(prev_coin, txin)};

        if (std::holds_alternative<CPubKey>(pubkey_variant)) {
            const auto& pubkey{std::get<CPubKey>(pubkey_variant)};
            if (pubkey.IsFullyValid()) {
                input_pubkeys.push_back(pubkey);
            }
        } else if (std::holds_alternative<XOnlyPubKey>(pubkey_variant)) {
            const auto& pubkey{std::get<XOnlyPubKey>(pubkey_variant)};
            if (pubkey.IsFullyValid()) {
                input_xonly_pubkeys.push_back(pubkey);
            }
        }
    }

    // Currently Silent Payment scheme uses all keys. If not possible to
    // retrieve all keys, it is not a SP transaction.
    if ((input_pubkeys.size() + input_xonly_pubkeys.size()) == tx.vin.size()) {
        sum_sender_pubkeys = CombinePublicKeys(input_pubkeys, input_xonly_pubkeys);
    }

    return sum_sender_pubkeys;
}

XOnlyPubKey Sender::Tweak(const XOnlyPubKey spend_xonly_pubkey) const
{
    return spend_xonly_pubkey.AddTweak(m_shared_secret);
}


Sender::Sender(const std::vector<std::tuple<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints, const XOnlyPubKey& recipient_scan_xonly_pubkey)
{
    const auto& [seckey, is_taproot] = sender_secret_keys.at(0);

    CKey sum_seckey{seckey};

    if (is_taproot && sum_seckey.GetPubKey()[0] == 3) {
        sum_seckey.Negate();
    }

    if (sender_secret_keys.size() > 1) {
        for (size_t i = 1; i < sender_secret_keys.size(); i++) {
            const auto& [sender_seckey, sender_is_taproot] = sender_secret_keys.at(i);

            auto temp_key{sender_seckey};
            if (sender_is_taproot && sender_seckey.GetPubKey()[0] == 3) {
                temp_key.Negate();
            }

            sum_seckey = sum_seckey.AddTweak(temp_key.begin());
        }
    }

    CPubKey recipient_scan_pubkey = recipient_scan_xonly_pubkey.ConvertToCompressedPubKey();

    const auto& [truncated_hash, outpoint_hash] = HashOutpoints(tx_outpoints);
    (void) truncated_hash; //not used here

    auto tweaked_sum_seckey = sum_seckey.MultiplyTweak(outpoint_hash.begin());
    std::array<unsigned char, 32> result = tweaked_sum_seckey.ECDH(recipient_scan_pubkey);

    std::copy(std::begin(result), std::end(result), std::begin(m_shared_secret));
}

std::pair<std::array<uint8_t, 8>, uint256> HashOutpoints(const std::vector<COutPoint>& tx_outpoints)
{
    uint256 result_hash;

    auto hash256 = CSHA256();

    for (const auto& outpoint: tx_outpoints) {
        hash256 = hash256.Write(std::begin(outpoint.hash), outpoint.hash.size());

        uint256 hash_n = uint256(outpoint.n);
        hash256 = hash256.Write(std::begin(hash_n), hash_n.size());
    }

    hash256.Finalize(result_hash.begin());

    std::array<uint8_t, 8> truncated_hash;

    for (std::size_t i{ 0 }; i < truncated_hash.size(); ++i) {
        truncated_hash[i] = result_hash.data()[i];
    }

    uint256 final_hash;
    CSHA256().Write(truncated_hash.data(), truncated_hash.size()).Finalize(final_hash.begin());

    return {truncated_hash, final_hash};
}

std::variant<CPubKey, XOnlyPubKey> ExtractPubkeyFromInput(const Coin& prevCoin, const CTxIn& txin)
{
    // scriptPubKey being spent by this input
    CScript scriptPubKey = prevCoin.out.scriptPubKey;

    if (scriptPubKey.IsPayToWitnessScriptHash()) {
        return CPubKey(); // returns an invalid pubkey
    }

    // Vector of parsed pubkeys and hashes
    std::vector<std::vector<unsigned char>> solutions;

    TxoutType whichType = Solver(scriptPubKey, solutions);

    if (whichType == TxoutType::NONSTANDARD ||
    whichType == TxoutType::MULTISIG ||
    whichType == TxoutType::WITNESS_UNKNOWN ) {
        return CPubKey(); // returns an invalid pubkey
    }

    const CScript scriptSig = txin.scriptSig;
    const CScriptWitness scriptWitness = txin.scriptWitness;

    // TODO: Condition not tested
    if (whichType == TxoutType::PUBKEY) {

        CPubKey pubkey = CPubKey(solutions[0]);
        assert(pubkey.IsFullyValid());
        return pubkey;
    }

    else if (whichType == TxoutType::PUBKEYHASH) {

        int sigSize = static_cast<int>(scriptSig[0]);
        int pubKeySize = static_cast<int>(scriptSig[sigSize + 1]);
        auto serializedPubKey = std::vector<unsigned char>(scriptSig.begin() + sigSize + 2, scriptSig.end());
        assert(serializedPubKey.size() == (size_t) pubKeySize);

        CPubKey pubkey = CPubKey(serializedPubKey);
        assert(pubkey.IsFullyValid());

        return pubkey;

    }

    else if (whichType == TxoutType::WITNESS_V0_KEYHASH || scriptPubKey.IsPayToScriptHash()) {
        if (scriptWitness.stack.size() != 2) return CPubKey(); // returns an invalid pubkey

        if (scriptWitness.stack.at(1).size() != 33) {
            return CPubKey();
        }

        CPubKey pubkey = CPubKey(scriptWitness.stack.at(1));
        assert(pubkey.IsFullyValid());

        return pubkey;
    }

    else if (whichType == TxoutType::WITNESS_V1_TAPROOT) {

        XOnlyPubKey pubkey = XOnlyPubKey(solutions[0]);
        assert(pubkey.IsFullyValid());
        return pubkey;
    }

    return CPubKey(); // returns an invalid pubkey
}

std::vector<std::tuple<uint256, CPubKey, uint256, std::array<uint8_t, 8>>> GetSilentPaymentKeysPerBlock(const uint256& block_hash, const CBlockUndo& blockUndo, const std::vector<CTransactionRef> vtx)
{
    std::vector<std::tuple<uint256, CPubKey, uint256, std::array<uint8_t, 8>>>  items; // <tx_hash, sum of public keys of transaction inputs, hash of the outpoints >

    for (const auto& tx : vtx) {

        if (tx->IsCoinBase()) {
            continue;
        }

        std::unordered_set<TxoutType> tx_vout_types;
        for (auto& vout : tx->vout) {
            std::vector<std::vector<unsigned char>> solutions;
            TxoutType whichType = Solver(vout.scriptPubKey, solutions);
            tx_vout_types.insert(whichType);
        }

        // Silent Payments require that the recipients use Taproot address
        // so one output at least must be Taproot
        if (tx_vout_types.find(TxoutType::WITNESS_V1_TAPROOT) == tx_vout_types.end()) {
            continue;
        }

        auto it = std::find_if(vtx.cbegin(), vtx.cend(), [tx](CTransactionRef t){ return *t == *tx; });
        // TODO: redundant verification ?
        if (it == vtx.end()) {
            continue;
        }

        // -1 as blockundo does not have coinbase tx
        const auto& undoTX = blockUndo.vtxundo.at(it - vtx.begin() - 1);

        assert(tx->vin.size() == undoTX.vprevout.size());

        std::vector<COutPoint> tx_outpoints;

        std::vector<Coin> coins;
        for (size_t i = 0; i < tx->vin.size(); i++)
        {
            coins.push_back(undoTX.vprevout[i]);
            tx_outpoints.push_back(tx->vin[i].prevout);
        }

        CPubKey sum_tx_pubkeys = silentpayment::Recipient::CombinePublicKeys(*tx, coins);

        const auto& [truncated_hash, outpoint_hash] = HashOutpoints(tx_outpoints);
        (void) truncated_hash; //not used here

        if (sum_tx_pubkeys.IsFullyValid()) {
            items.emplace_back(tx->GetHash(), sum_tx_pubkeys, outpoint_hash, truncated_hash);
        }
    }

    return items;
}
} // namespace silentpayment
