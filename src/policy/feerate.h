// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEERATE_H
#define BITCOIN_POLICY_FEERATE_H

#include "amount.h"
#include "serialize.h"

#include <string>

/**
 * Fee rate in satoshis per weight unit: CAmount / WU
 */
class CFeeRate
{
private:
    CAmount nSatoshisPerWU; // unit is satoshis-per-weight-unit

public:
    /** Fee rate of 0 satoshis per WU */
    CFeeRate() : nSatoshisPerWU(0) { }
    template<typename I>
    CFeeRate(const I _nSatoshisPerWU): nSatoshisPerWU(_nSatoshisPerWU) {
        // We've previously had bugs creep in from silent double->int conversion...
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }
    /** Constructor for a fee rate in satoshis per WU. The size in weight units must not exceed (2^63 - 1)*/
    CFeeRate(const CAmount& nFeePaid, size_t nWeightUnits);
    /**
     * Return the fee in satoshis for the given size in weight units.
     */
    CAmount GetFee(size_t nWeightUnits) const;
    /**
     * Return the fee in satoshis for a size of 4000 weight units
     */
    CAmount GetFeePerWU() const { return GetFee(4000); }
    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU < b.nSatoshisPerWU; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU > b.nSatoshisPerWU; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU == b.nSatoshisPerWU; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU <= b.nSatoshisPerWU; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU >= b.nSatoshisPerWU; }
    friend bool operator!=(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerWU != b.nSatoshisPerWU; }
    CFeeRate& operator+=(const CFeeRate& a) { nSatoshisPerWU += a.nSatoshisPerWU; return *this; }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nSatoshisPerWU);
    }
};

#endif //  BITCOIN_POLICY_FEERATE_H
