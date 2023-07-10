// Copyright (c) 2017-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SIDECHAIN_H
#define BITCOIN_SIDECHAIN_H

#include <serialize.h>
#include <uint256.h>

#include <cstdint>
#include <string>

class CTransaction;
class CCoinsViewCache;
class CTxUndo;

//! The current sidechain version
static constexpr int SIDECHAIN_VERSION_CURRENT{0};

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

struct Sidechain {
    uint8_t idnum{0};
    int32_t version{SIDECHAIN_VERSION_CURRENT};
    std::string title;
    std::string description;
    uint256 hashID1;
    uint160 hashID2;

    SERIALIZE_METHODS(Sidechain, obj) {
        READWRITE(obj.idnum, obj.version, obj.title, obj.description, obj.hashID1, obj.hashID2);
    }
};

void UpdateDrivechains(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo &txundo, int nHeight);

#endif // BITCOIN_SIDECHAIN_H
