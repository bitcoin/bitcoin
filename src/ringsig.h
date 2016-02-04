// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMEUNITS_RINGSIG_H
#define GAMEUNITS_RINGSIG_H

#include "stealth.h"
#include "state.h"


const uint32_t MIN_ANON_OUT_SIZE = 1 + 1 + 1 + 33 + 1 + 33; // OP_RETURN ANON_TOKEN lenPk pkTo lenR R [lenEnarr enarr]
const uint32_t MAX_ANON_NARRATION_SIZE = 48;
const uint32_t MIN_RING_SIZE = 3;
const uint32_t MAX_RING_SIZE = 200;

const int MIN_ANON_SPEND_DEPTH = 10;
const int ANON_TXN_VERSION = 1000;

// MAX_MONEY = 200000000000000000; most complex possible value can be represented by 36 outputs

int initialiseRingSigs();
int finaliseRingSigs();

int splitAmount(int64_t nValue, std::vector<int64_t>& vOut);

int generateKeyImage(ec_point &publicKey, ec_secret secret, ec_point &keyImage);


int generateRingSignature(std::vector<uint8_t>& keyImage, uint256& txnHash, int nRingSize, int nSecretOffset, ec_secret secret, const uint8_t *pPubkeys, uint8_t *pSigc, uint8_t *pSigr);

int verifyRingSignature(std::vector<uint8_t>& keyImage, uint256& txnHash, int nRingSize, const uint8_t *pPubkeys, const uint8_t *pSigc, const uint8_t *pSigr);


#endif  // GAMEUNITS_RINGSIG_H

