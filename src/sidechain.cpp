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
    const bool new_entry = s.empty();
    modify_func(s);
    if (!new_entry) DeleteDBEntry(view, txundo, record_id);
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

uint256 CalculateDrivechainWithdrawInternalHash(const uint256& blinded_hash, const uint8_t sidechain_id) {
    // Internally, we hash the bundle_hash with the sidechain_id to avoid conflicts between sidechains
    uint256 internal_hash;
    CSHA256().Write(blinded_hash.data(), blinded_hash.size()).Write(&sidechain_id, sizeof(sidechain_id)).Finalize(internal_hash.data());
    return internal_hash;
}

bool UpdateDrivechains(const CTransaction& tx, CCoinsViewCache& view, CTxUndo &txundo, int block_height, BlockValidationState& state)
{
    Assert(tx.IsCoinBase());

    std::vector<unsigned char> sidechain_proposal_list, withdraw_proposal_list;
    std::set<uint8_t> saw_withdraw_proposed_for_sidechain;
    bool proposed_a_sidechain{false}, saw_sidechain_acks{false};

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
                // NOTE data_format 2 changed to 0 FIXME
                // TODO: (new) data format 2 sets it to the ACKs from the previous block - but those aren't known, have the same cost, and encourages blind upvoting; so can we get rid of it?
                // TODO: data format 3 upvotes any bundle leading its rivals by at least 50 ACKs -- also encourages blind upvoting :/
            // TODO: How is vote vector actually encoded?
            // TODO: Block is invalid if there are no bundles proposed at all
            // FIXME: Presumably blocks should only be able to vote once - this is missing in the BIP
            for (int sidechain_id = 0; sidechain_id < 0x100; ++sidechain_id) {
                // FIXME: bounds checking
                // FIXME: skip votes for sidechains with no proposals
                uint16_t vote = out.scriptPubKey[6 + (sidechain_id * data_format)];
                if (data_format == 2) {
                    vote |= uint16_t{out.scriptPubKey[6 + (sidechain_id * data_format) + 1]} << 8;
                } else if (vote >= 0xfe) {
                    vote |= 0xff00;
                }

                if (vote == 0xffff) continue;  // abstain

                // FIXME: what if it's missing?
                CDataStream withdraw_proposals = GetDBEntry(view, {uint256{(uint8_t)sidechain_id}, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST});
                uint256 bundle_hash;
                bool found_bundle{false};
                for (uint16_t bundle_hash_num = 0; !withdraw_proposals.eof(); ++bundle_hash_num) {
                    withdraw_proposals >> bundle_hash;
                    if (bundle_hash_num == vote) found_bundle = true;
                    IncrementDBEntry(view, txundo, block_height, {bundle_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS}, (bundle_hash_num == vote) ? 1 : -1);
                }
                if ((!found_bundle) && vote != 0xfffe) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-withdraw-ack-nonexistent");
                }
            }
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_WITHDRAW_PROPOSE)) {
            if (out.scriptPubKey.size() != 0x26) {
                // "M3 is ignored if it does not parse"
                continue;
            }
            CDataStream s(MakeByteSpan(out.scriptPubKey).subspan(5), SER_NETWORK, PROTOCOL_VERSION);
            uint256 bundle_hash;
            uint8_t sidechain_id;
            try {
                s >> bundle_hash;
                s >> sidechain_id;
            } catch (...) {
                // "M3 is ignored if it does not parse"
                continue;
            }

            if (GetDBEntry(view, {uint256{sidechain_id}, DBIDX_SIDECHAIN_DATA}).empty()) {
                // "M3 is ignored...if it is for a sidechain that doesn't exist."
                continue;
            }

            // Internally, we hash the bundle_hash with the sidechain_id to avoid conflicts between sidechains
            // TODO: maybe define this in the BIP and M3 ?
            bundle_hash = CalculateDrivechainWithdrawInternalHash(bundle_hash, sidechain_id);

            // "M3 is invalid if...This block already has an M3 for that nSidechain."
            if (saw_withdraw_proposed_for_sidechain.find(sidechain_id) != saw_withdraw_proposed_for_sidechain.end()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-withdraw-propose-multiple");
            }
            saw_withdraw_proposed_for_sidechain.insert(sidechain_id);

            // FIXME: "M3 is invalid if...A bundle with this hash already paid out. A bundle with this hash was rejected in the past." is not practical to track!

            if (!GetDBEntry(view, {bundle_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS}).empty()) {
                // Withdraw has already been proposed, invalid
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-withdraw-propose-duplicate");
            }

            ModifyDBEntry(view, txundo, block_height, {uint256{sidechain_id}, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST}, [&bundle_hash](CDataStream& withdraw_proposals){
                withdraw_proposals << bundle_hash;
            });

            s.clear();
            s << uint16_t{0};
            CreateDBEntry(view, txundo, block_height, {bundle_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS}, s);

            withdraw_proposal_list.resize(withdraw_proposal_list.size() + bundle_hash.size());
            memcpy(&withdraw_proposal_list.data()[withdraw_proposal_list.size() - bundle_hash.size()], bundle_hash.data(), bundle_hash.size());  // FIXME: C++ify
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_SIDECHAIN_ACK)) {
            if (saw_sidechain_acks) {
                // FIXME: shouldn't it be possible to ACK multiple proposals for different sidechain ids??
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-sidechain-ack-multiple");
            }
            saw_sidechain_acks = true;

            if (out.scriptPubKey.size() != 0x25) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-sidechain-ack-unparsable");
            }
            const uint256 sidechain_proposal_hash{Span{&out.scriptPubKey[5], 0x20}};
            try {
                IncrementDBEntry(view, txundo, block_height, {sidechain_proposal_hash, DBIDX_SIDECHAIN_PROPOSAL_ACKS}, 1);
            } catch (...) {  // TODO: make this explicitly for a non-existent entry
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-sidechain-ack-unknown");
            }
        } else if (std::equal(&out.scriptPubKey[1], &out.scriptPubKey[5], BIP300_HEADER_SIDECHAIN_PROPOSE)) {
            if (proposed_a_sidechain) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-sidechain-propose-multiple");
            }
            proposed_a_sidechain = true;

            CDataStream s(MakeByteSpan(out.scriptPubKey).subspan(5), SER_NETWORK, PROTOCOL_VERSION);
            Sidechain proposed;
            try {
                s >> proposed;
            } catch (...) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-sidechain-propose-unparsable");
            }

            // TODO: block is invalid if proposed matches the current sidechain

            uint256 sidechain_proposal_hash;
            CSHA256().Write(out.scriptPubKey.data() + 5, out.scriptPubKey.size() - 5).Finalize(sidechain_proposal_hash.data());
            CreateDBEntry(view, txundo, block_height, {sidechain_proposal_hash, DBIDX_SIDECHAIN_PROPOSAL}, s);

            s.clear();
            s << uint16_t{0};
            CreateDBEntry(view, txundo, block_height, {sidechain_proposal_hash, DBIDX_SIDECHAIN_PROPOSAL_ACKS}, s);

            sidechain_proposal_list.resize(sidechain_proposal_list.size() + sidechain_proposal_hash.size());
            memcpy(&sidechain_proposal_list.data()[sidechain_proposal_list.size() - sidechain_proposal_hash.size()], sidechain_proposal_hash.data(), sidechain_proposal_hash.size());  // FIXME: C++ify
        }
    }

    if (!(sidechain_proposal_list.empty() && withdraw_proposal_list.empty())) {
        CDataStream proposal_list(SER_NETWORK, PROTOCOL_VERSION);
        proposal_list << sidechain_proposal_list;
        proposal_list << withdraw_proposal_list;
        CreateDBEntry(view, txundo, block_height, {ArithToUint256(arith_uint256{(uint64_t)block_height}), DBIDX_SIDECHAIN_PROPOSAL_LIST}, proposal_list);
    }

    // Perform sidechain overwriting/expiry and withdraw expiry
    int completed_block_height = block_height - (SIDECHAIN_WITHDRAW_PERIOD - 1);
    COutPoint record_id{ArithToUint256(arith_uint256{(uint64_t)completed_block_height}), DBIDX_SIDECHAIN_PROPOSAL_LIST};
    CDataStream completed_proposal_list = GetDBEntry(view, record_id);
    if (!completed_proposal_list.empty()) {
        DeleteDBEntry(view, txundo, record_id);
        completed_proposal_list >> sidechain_proposal_list;
        completed_proposal_list >> withdraw_proposal_list;

        for (size_t i = 0; i < sidechain_proposal_list.size(); i += uint256::size()) {
            uint256 sidechain_proposal_hash{Span{&sidechain_proposal_list[i], uint256::size()}};
            record_id.hash = sidechain_proposal_hash;
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL_ACKS;
            uint16_t acks;
            {
                CDataStream acks_s = GetDBEntry(view, record_id);
                Assert(!acks_s.empty());
                acks_s >> acks;
            }
            if (acks >= SIDECHAIN_WITHDRAW_THRESHOLD) {
                // Overwrite an existing sidechain with a new one
                CDataStream proposal_s = GetDBEntry(view, {sidechain_proposal_hash, DBIDX_SIDECHAIN_PROPOSAL});
                Assert(!proposal_s.empty());
                Sidechain proposal;
                proposal_s >> proposal;

                COutPoint sidechain_record_id{uint256{proposal.idnum}, DBIDX_SIDECHAIN_DATA};
                DeleteDBEntry(view, txundo, sidechain_record_id);
                CreateDBEntry(view, txundo, block_height, sidechain_record_id, proposal_s);
            }
            DeleteDBEntry(view, txundo, record_id);
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL;
            DeleteDBEntry(view, txundo, record_id);
        }

        for (size_t i = 0; i < withdraw_proposal_list.size(); i += uint256::size()) {
            uint256 withdraw_proposal_hash{Span{&withdraw_proposal_list[i], uint256::size()}};
            // FIXME: remove from DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST
            DeleteDBEntry(view, txundo, {withdraw_proposal_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS});
        }
    }

    // New sidechain activation
    completed_block_height = block_height - (SIDECHAIN_ACTIVATION_PERIOD - 1);
    completed_proposal_list = GetDBEntry(view, {ArithToUint256(arith_uint256{(uint64_t)completed_block_height}), DBIDX_SIDECHAIN_PROPOSAL_LIST});
    if (!completed_proposal_list.empty()) {
        completed_proposal_list >> sidechain_proposal_list;
        completed_proposal_list >> withdraw_proposal_list;

        std::vector<unsigned char> sidechain_proposal_list_new;
        std::set<uint8_t> new_sidechains_activated;
        for (size_t i = 0; i < sidechain_proposal_list.size(); i += uint256::size()) {
            uint256 sidechain_proposal_hash{Span{&sidechain_proposal_list[i], uint256::size()}};
            CDataStream proposal_s = GetDBEntry(view, {sidechain_proposal_hash, DBIDX_SIDECHAIN_PROPOSAL});
            Assert(!proposal_s.empty());
            Sidechain proposal;
            proposal_s >> proposal;

            COutPoint sidechain_record_id{uint256{proposal.idnum}, DBIDX_SIDECHAIN_DATA};
            CDataStream old_sidechain_s = GetDBEntry(view, sidechain_record_id);
            if (!old_sidechain_s.empty()) {
                // This would be an overwrite, so it must wait for the final completion after SIDECHAIN_WITHDRAW_PERIOD
                sidechain_proposal_list_new.resize(sidechain_proposal_list_new.size() + sidechain_proposal_hash.size());
                memcpy(&sidechain_proposal_list_new.data()[sidechain_proposal_list_new.size() - sidechain_proposal_hash.size()], sidechain_proposal_hash.data(), sidechain_proposal_hash.size());  // FIXME: C++ify
                continue;
            }

            record_id.hash = sidechain_proposal_hash;
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL_ACKS;
            uint16_t acks;
            {
                CDataStream acks_s = GetDBEntry(view, record_id);
                Assert(!acks_s.empty());
                acks_s >> acks;
            }
            if (acks >= SIDECHAIN_ACTIVATION_THRESHOLD) {
                // Activate new sidechain
                CreateDBEntry(view, txundo, block_height, sidechain_record_id, proposal_s);
                new_sidechains_activated.insert(proposal.idnum);
            }
            DeleteDBEntry(view, txundo, record_id);
            record_id.n = DBIDX_SIDECHAIN_PROPOSAL;
            DeleteDBEntry(view, txundo, record_id);
        }

        if (!new_sidechains_activated.empty()) {
            Assume(sidechain_proposal_list.size() != sidechain_proposal_list_new.size());
            // Assign CTIPs
            for (size_t output_index = 0; output_index < tx.vout.size(); ++output_index) {
                if (!tx.vout[output_index].scriptPubKey.IsDrivechain()) continue;

                const uint8_t sidechain_id = tx.vout[output_index].scriptPubKey[DRIVECHAIN_SCRIPT_SIDECHAIN_ID_OFFSET];
                if (new_sidechains_activated.find(sidechain_id) == new_sidechains_activated.end()) {
                    // TODO: OP_DRIVECHAIN (or OP_NOP5) for a non-activating sidechain id; should this be invalid??
                    continue;
                }

                CDataStream ctip_info(SER_NETWORK, PROTOCOL_VERSION);
                ctip_info << sidechain_id;
                CreateDBEntry(view, txundo, block_height, {tx.GetHash(), DBIDX_SIDECHAIN_CTIP_INFO}, ctip_info);
            }
            if (!new_sidechains_activated.empty()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-drivechain-activated-without-ctip");
            }
        }

        if (sidechain_proposal_list.size() != sidechain_proposal_list_new.size()) {
            Assume(!new_sidechains_activated.empty());
            COutPoint record_id{ArithToUint256(arith_uint256{(uint64_t)completed_block_height}), DBIDX_SIDECHAIN_PROPOSAL_LIST};
            DeleteDBEntry(view, txundo, record_id);

            if (!(sidechain_proposal_list_new.empty() && withdraw_proposal_list.empty())) {
                CDataStream proposal_list(SER_NETWORK, PROTOCOL_VERSION);
                proposal_list << sidechain_proposal_list_new;
                proposal_list << withdraw_proposal_list;
                CreateDBEntry(view, txundo, block_height, record_id, proposal_list);
            }
        }
    }

    return true;
}

bool VerifyDrivechainSpend(const CTransaction& tx, const unsigned int sidechain_input_n, const std::vector<CTxOut>& spent_outputs, const CCoinsViewCache& view, TxValidationState& state) {
    const CTxIn& sidechain_input = tx.vin[sidechain_input_n];
    // TODO: Do we want to verify there's only one sidechain involved? BIP300 says yes, but why?

    // Lookup sidechain number from CTIP and ensure this is in fact a CTIP to begin with
    // FIXME: It might be a good idea to include the sidechain # in the tx itself somewhere?
    uint8_t sidechain_id;
    {
        CDataStream ctip_info = GetDBEntry(view, {sidechain_input.prevout.hash, DBIDX_SIDECHAIN_CTIP_INFO});
        if (ctip_info.empty()) {
            // Not an active CTIP, so treat as NOP5
            // FIXME: This could be abused to bypass the extra OP_DRIVECHAIN weight
            return true;
        }
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
    CScript ctip_output_script{OP_DRIVECHAIN};
    ctip_output_script.push_back(1);
    ctip_output_script.push_back(sidechain_id);
    ctip_output_script << OP_TRUE;
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

    const uint256 internal_hash = CalculateDrivechainWithdrawInternalHash(CalculateDrivechainWithdrawBlindedHash(tx), sidechain_id);

    // TODO: Ensure bundle hash is actually for expected sidechain id

    CDataStream s = GetDBEntry(view, {internal_hash, DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS});
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
