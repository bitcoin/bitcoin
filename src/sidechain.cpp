// Copyright (c) 2017-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sidechain.h>

#include <arith_uint256.h>
#include <coins.h>
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

CDataStream GetDBEntry(CCoinsViewCache& inputs, const COutPoint& record_id) {
    const Coin& coin = inputs.AccessCoin(record_id);
    assert(!coin.IsSpent());
    return CDataStream(MakeByteSpan(coin.out.scriptPubKey), SER_NETWORK, PROTOCOL_VERSION);
}

void ModifyDBEntry(CCoinsViewCache& view, CTxUndo &txundo, const int block_height, const COutPoint& record_id, const std::function<void(CDataStream&)>& modify_func) {
    CDataStream s = GetDBEntry(view, record_id);
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
}
