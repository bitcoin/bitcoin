// Copyright (c) 2017-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sidechain.h>

#include <arith_uint256.h>
#include <coins.h>
#include <consensus/validation.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <undo.h>
#include <util/check.h>

#include <algorithm>
#include <cstdint>

void CreateDBUndoData(CTxUndo &txundo, const uint8_t type, const COutPoint& record_id, const Coin& encoded_data) {
    Assert(record_id.n <= COutPoint::MAX_INDEX);
    Coin& undo = txundo.vprevout.emplace_back();
    undo = encoded_data;
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << record_id;
    Assert(s.size() == 0x24);
    auto& undo_data = undo.out.scriptPubKey;
    undo_data.insert(undo_data.begin(), 1 + s.size(), type);
    memcpy(&undo_data[1], &s[0], s.size());  // TODO: figure out how to jump through C++'s hoops to do this right
}

void CreateDBEntry(CCoinsViewCache& cache, CTxUndo &txundo, const int block_height, const COutPoint& record_id, const Span<const std::byte>& record_data) {
    CScript scriptPubKey(UCharCast(record_data.begin()), UCharCast(record_data.end()));
    cache.AddCoin(record_id, Coin(CTxOut{0, scriptPubKey}, block_height, /*fCoinbase=*/false), /*overwrite=*/false);

    // Create undo data to tell DisconnectBlock to delete it
    Coin undo;
    CreateDBUndoData(txundo, 1, record_id, undo);
}

void DeleteDBEntry(CCoinsViewCache& inputs, CTxUndo &txundo, const COutPoint& record_id) {
    Coin undo;
    bool is_spent = inputs.SpendCoin(record_id, &undo);
    assert(is_spent);
    CreateDBUndoData(txundo, 0, record_id, undo);
}

CDataStream GetDBEntry(const CCoinsViewCache& inputs, const COutPoint& record_id) {
    const Coin& coin = inputs.AccessCoin(record_id);
    return CDataStream(MakeByteSpan(coin.out.scriptPubKey), SER_NETWORK, PROTOCOL_VERSION);
}

void ModifyDBEntry(CCoinsViewCache& view, CTxUndo &txundo, const int block_height, const COutPoint& record_id, const std::function<void(CDataStream&)>& modify_func) {
    CDataStream s = GetDBEntry(view, record_id);
    assert(!s.empty());
    modify_func(s);
    DeleteDBEntry(view, txundo, record_id);
    CreateDBEntry(view, txundo, block_height, record_id, s);
}

void IncrementDBEntry(CCoinsViewCache& view, CTxUndo &txundo, const int block_height, const COutPoint& record_id, const int change) {
    ModifyDBEntry(view, txundo, block_height, record_id, [change](CDataStream& s){
        uint16_t counter;
        s >> counter;
        if (change < 0 && !counter) return;  // may be surprising if change is <-1
        counter += change;
        s.clear();
        s << counter;
    });
}

uint256 CalculateDrivechainWithdrawBlindedHash(const CTransaction& tx) {
    CMutableTransaction mtx(tx);
    mtx.vin[0].SetNull();
    mtx.vout[0].SetNull();
    return mtx.GetHash();
}

void UpdateDrivechains(const CTransaction& tx, CCoinsViewCache& view, CTxUndo &txundo, int block_height)
{
    Assert(tx.IsCoinBase());

    std::vector<unsigned char> sidechain_proposal_list, withdraw_proposal_list;

    for (auto& out : tx.vout) {
        if (out.scriptPubKey.size() < 5) continue;
        if (out.scriptPubKey[0] != OP_RETURN) continue;
        // FIXME: The rest should probably be serialised, but neither BIP300 nor its reference implementation does that
        static constexpr uint8_t BIP300_HEADER_SIDECHAIN_PROPOSE[] = {0xd5, 0xe0, 0xc4, 0xaf};  // n.b. 20 sigops
        static constexpr uint8_t BIP300_HEADER_SIDECHAIN_ACK[]     = {0xd6, 0xe1, 0xc5, 0xbf};
        static constexpr uint8_t BIP300_HEADER_WITHDRAW_PROPOSE[]  = {0xd4, 0x5a, 0xa9, 0x43};  // n.b. 67 byte push followed by only 32 bytes
        static constexpr uint8_t BIP300_HEADER_WITHDRAW_ACK[]      = {0xd7, 0x7d, 0x17, 0x76};  // n.b. 23-byte push followed by variable bytes
        if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_WITHDRAW_ACK)) {
            const uint8_t data_format = out.scriptPubKey[6];
            // TODO: Implement formats 3+? Or at least validate
            // TODO: How is vote vector actually encoded?
            // TODO: Block is invalid if there are no bundles proposed at all
            for (int sidechain_id = 0; sidechain_id < 0x100; ++sidechain_id) {
                // FIXME: bounds checking
                uint16_t vote = out.scriptPubKey[6 + (sidechain_id * data_format)];
                if (data_format == 2) {
                    vote |= uint16_t{out.scriptPubKey[6 + (sidechain_id * data_format) + 1]} << 8;
                } else if (vote >= 0xfe) {
                    vote |= 0xff00;
                }

                if (vote == 0xffff) continue;  // abstain

                COutPoint record_id{uint256{sidechain_id}, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST};
                // FIXME: what if it's missing?
                CDataStream withdraw_proposals = GetDBEntry(view, record_id);
                uint256 bundle_hash;
                bool found_bundle{false};
                for (uint16_t bundle_hash_num = 0; !withdraw_proposals.eof(); ++bundle_hash_num) {
                    withdraw_proposals >> bundle_hash;
                    record_id.hash = bundle_hash;
                    record_id.n = DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS;
                    if (bundle_hash_num == vote) found_bundle = true;
                    IncrementDBEntry(view, txundo, block_height, record_id, (bundle_hash_num == vote) ? 1 : -1);
                }
                // TODO: invalid if ((!found_bundle) && vote != 0xfffe)
            }
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_WITHDRAW_PROPOSE)) {
            // FIXME; size check; [at least] 38 bytes
            CDataStream s(MakeByteSpan(out.scriptPubKey).subspan(5), SER_NETWORK, PROTOCOL_VERSION);
            uint256 bundle_hash;
            uint8_t sidechain_id;
            s >> bundle_hash;
            s >> sidechain_id;

            // FIXME: make sure sidechain_id hasn't been encountered here in this block before
            // FIXME: make sure this proposal isn't already listed (check for ACKS existing?)
            // FIXME: allow the same bundle for multiple sidechain_id to prevent DoS attacks?

            COutPoint record_id{uint256{sidechain_id}, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST};
            ModifyDBEntry(view, txundo, block_height, record_id, [&bundle_hash](CDataStream& withdraw_proposals){
                withdraw_proposals << bundle_hash;
            });

            record_id.hash = bundle_hash;
            record_id.n = DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS;
            s.clear();
            s << uint16_t{0};
            CreateDBEntry(view, txundo, block_height, record_id, s);

            withdraw_proposal_list.resize(withdraw_proposal_list.size() + record_id.hash.size());
            memcpy(&withdraw_proposal_list.data()[withdraw_proposal_list.size() - record_id.hash.size()], record_id.hash.data(), record_id.hash.size());  // FIXME: C++ify
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_SIDECHAIN_ACK)) {
            // FIXME: check size is [at least?] 37 bytes
            COutPoint record_id;
            memcpy(record_id.hash.data(), &out.scriptPubKey[5], 0x20);
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL_ACKS;

            IncrementDBEntry(view, txundo, block_height, record_id, 1);
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_SIDECHAIN_PROPOSE)) {
            CDataStream s(MakeByteSpan(out.scriptPubKey).subspan(5), SER_NETWORK, PROTOCOL_VERSION);
            Sidechain proposed;
            // FIXME: What happens if parsing fails?
            s >> proposed;

            COutPoint record_id;
            CSHA256().Write(out.scriptPubKey.data() + 5, out.scriptPubKey.size() - 5).Finalize(record_id.hash.data());
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL;
            CreateDBEntry(view, txundo, block_height, record_id, s);

            s.clear();
            s << uint16_t{0};
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL_ACKS;
            CreateDBEntry(view, txundo, block_height, record_id, s);

            sidechain_proposal_list.resize(sidechain_proposal_list.size() + record_id.hash.size());
            memcpy(&sidechain_proposal_list.data()[sidechain_proposal_list.size() - record_id.hash.size()], record_id.hash.data(), record_id.hash.size());  // FIXME: C++ify
        }
    }

    if (sidechain_proposal_list.empty() && withdraw_proposal_list.empty()) return;

    CScript proposal_list;
    proposal_list << sidechain_proposal_list;
    proposal_list << withdraw_proposal_list;
    COutPoint record_id{ArithToUint256(arith_uint256{uint64_t{block_height}}), DBIDX_SIDECHAIN_PROPOSAL_LIST};
    CreateDBEntry(view, txundo, block_height, record_id, MakeByteSpan(proposal_list));

    // TODO: clean up expiring stuff
    // TODO: activate sidechains and assign CTIPs
}

bool VerifyDrivechainSpend(const CTransaction& tx, const unsigned int sidechain_input_n, const std::vector<CTxOut>& spent_outputs, const CCoinsViewCache& view, TxValidationState& state) {
    const CTxIn& sidechain_input = tx.vin[sidechain_input_n];
    // TODO: Do we want to verify there's only one sidechain involved? BIP300 says yes, but why?

    // Lookup sidechain number from CTIP and ensure this is in fact a CTIP to begin with
    // FIXME: It might be a good idea to include the sidechain # in the tx itself somewhere?
    {
        COutPoint record_id{sidechain_input.prevout.hash, DBIDX_SIDECHAIN_CTIP_INFO};
        CDataStream ctip_info = GetDBEntry(view, record_id);
        if (ctip_info.empty()) {
            // Not an active CTIP, so treat as NOP5
            // FIXME: This could be abused to bypass the extra OP_DRIVECHAIN weight
            return true;
        }
        uint8_t sidechain_id;
        ctip_info >> sidechain_id;

        {
            uint32_t ctip_outpoint_index;
            ctip_info >> ctip_outpoint_index;
            if (ctip_outpoint_index != sidechain_input.prevout.n) {
                // Not an active CTIP (another index is), so treat as NOP5
                return true;
            }
        }
    }

    // Identify new CTIP output
    unsigned int sidechain_output_n = (unsigned int)-1;
    static const CScript ctip_output_script{OP_DRIVECHAIN};
    for (unsigned int i = 0; i < tx.vout.size(); ++i) {
        if (tx.vout[i].scriptPubKey != ctip_output_script) continue;

        if (sidechain_output_n == (unsigned int)-1) {
            sidechain_output_n = i;
        } else {
            // Multiple sidechain outputs is invalid?
            // FIXME: Add to BIP
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-ctip-output-multiple");
        }
    }
    if (sidechain_output_n == (unsigned int)-1) {
        // There must always be a new CTIP, so this is invalid
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-ctip-output-missing");
    }
    const CTxOut& sidechain_output = tx.vout[sidechain_output_n];

    // If output > input, transaction doesn't need any additional checks
    // FIXME: Define what should happen if output amt==input amt exactly
    if (sidechain_output.nValue >= spent_outputs[sidechain_input_n].nValue) {
        return true;
    }

    // Sidechain Withdraw

    if (sidechain_output_n != 0) {
        // Withdraws must put the new CTIP at index 0 (FIXME: why? if changing, adjust CalculateDrivechainWithdrawBlindedHash assumption)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-ctip-output-nonzero");
    }

    if (tx.vout.size() < 2) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-fee-output-missing");
    }

    if (tx.vout[1].nValue != 0) {
        // Ensure the sidechain coins can't be burned in the fee commitment
        // TODO: Document in BIP
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-fee-output-hasvalue");
    }

    // Ensure transaction fee matches OP_RETURN data in 2nd output
    {
        CAmount fee = -tx.GetValueOut();
        for (const auto& txout : spent_outputs) {
            fee += txout.nValue;
        }
        Assert(fee >= 0);

        std::vector<unsigned char> fee_data(8, 0);
        WriteLE64(fee_data.data(), fee);

        CScript fee_output_script;
        fee_output_script << OP_RETURN << fee_data;

        if (tx.vout[1].scriptPubKey != fee_output_script) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-fee-output-incorrect");
        }
    }

    const uint256 blinded_hash = CalculateDrivechainWithdrawBlindedHash(tx);

    // TODO: Ensure bundle hash is actually for expected sidechain id

    COutPoint record_id{blinded_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS};
    CDataStream s = GetDBEntry(view, record_id);
    if (s.empty()) {
        // No proposed withdraw, invalid
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-withdraw-not-proposed");
    }
    uint16_t counter;
    s >> counter;
    if (counter < 13150) {
        // Not enough ACKs, invalid
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-drivechain-withdraw-acks-insufficient");
    }

    return true;
}
