// Copyright (c) 2017-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SIDECHAIN_H
#define BITCOIN_SIDECHAIN_H

#include <serialize.h>
#include <uint256.h>

#include <cstdint>
#include <string>
#include <vector>

class CBlock;
class CCoinsViewCache;
class CTransaction;
class CTxOut;
class CTxUndo;
class TxValidationState;

//! The current sidechain version
static constexpr int SIDECHAIN_VERSION_CURRENT{0};

// Number of blocks for a new sidechain
static constexpr int SIDECHAIN_ACTIVATION_PERIOD = 2016;
static constexpr int SIDECHAIN_ACTIVATION_THRESHOLD = SIDECHAIN_ACTIVATION_PERIOD - 200;
// Number of blocks a sidechain withdraw (or overwrite) can be valid (after acquiring enough ACKs)
static constexpr int SIDECHAIN_WITHDRAW_PERIOD = 26300;
static constexpr int SIDECHAIN_WITHDRAW_THRESHOLD = SIDECHAIN_WITHDRAW_PERIOD / 2;

// Key is the sidechain number; Data is the Sidechain itself
static constexpr uint32_t DBIDX_SIDECHAIN_DATA{0xff010006};
// Key is the proposal hash; Data is the proposal itself
static constexpr uint32_t DBIDX_SIDECHAIN_PROPOSAL{0xff010000};
// Key is the block height; Data is a serialised list of hashes of sidechain proposals in the block, then a serialised list of withdraw proposals in the block
static constexpr uint32_t DBIDX_SIDECHAIN_PROPOSAL_LIST{0xff010001};
// Key is the proposal hash; Data is a uint16_t with ACK count
static constexpr uint32_t DBIDX_SIDECHAIN_PROPOSAL_ACKS{0xff010002};
// Key is the sidechain number; Data is a raw list of blinded-hashes of withdraw proposals
static constexpr uint32_t DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_LIST{0xff010003};
// Key is a blinded withdrawl hash; Data is a uint16_t with ACK count
static constexpr uint32_t DBIDX_SIDECHAIN_WITHDRAW_PROPOSAL_ACKS{0xff010004};
// Key is a CTIP; data is uint8 sidechain id it's for and uint32 output index
static constexpr uint32_t DBIDX_SIDECHAIN_CTIP_INFO{0xff010005};

struct Sidechain {
    uint8_t idnum{0};
    int32_t version{SIDECHAIN_VERSION_CURRENT};
    std::string title;
    std::string description;

    SERIALIZE_METHODS(Sidechain, obj) {
        READWRITE(obj.idnum, obj.version, obj.title, obj.description);
    }
};

void UpdateDrivechains(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo &txundo, int nHeight);
bool VerifyDrivechainSpend(const CTransaction& tx, unsigned int sidechain_input_n, const std::vector<CTxOut>& spent_outputs, const CCoinsViewCache& view, TxValidationState& state);

#endif // BITCOIN_SIDECHAIN_H
