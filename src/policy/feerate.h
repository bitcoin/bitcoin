// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEERATE_H
#define BITCOIN_POLICY_FEERATE_H

#include <consensus/amount.h>
#include <serialize.h>
#include <util/feefrac.h>


#include <cstdint>
#include <string>
#include <type_traits>

const std::string CURRENCY_UNIT = "BTC"; // One formatted unit
const std::string CURRENCY_ATOM = "sat"; // One indivisible minimum value unit

/* Used to determine type of fee estimation requested */
enum class FeeEstimateMode {
    UNSET,        //!< Use default settings based on other criteria
    ECONOMICAL,   //!< Force estimateSmartFee to use non-conservative estimates
    CONSERVATIVE, //!< Force estimateSmartFee to use conservative estimates
    BTC_KVB,      //!< Use BTC/kvB fee rate unit
    SAT_VB,       //!< Use sat/vB fee rate unit
};

/**
 * Fee rate in satoshis per virtualbyte: CAmount / vB
 * the feerate is represented internally as FeeFrac
 */
class CFeeRate
{
private:
    /** Fee rate in sats/vB (satoshis per N virtualbytes) */
    FeePerVSize m_feerate;

public:
    /** Fee rate of 0 satoshis per 0 vB */
    CFeeRate() = default;
    template<std::integral I> // Disallow silent float -> int conversion
    explicit CFeeRate(const I m_feerate_kvb) : m_feerate(FeePerVSize(m_feerate_kvb, 1000)) {}

    /**
     * Construct a fee rate from a fee in satoshis and a vsize in vB.
     *
     * param@[in]   nFeePaid    The fee paid by a transaction, in satoshis
     * param@[in]   virtual_bytes   The vsize of a transaction, in vbytes
     *
     * Passing any virtual_bytes less than or equal to 0 will result in 0 fee rate per 0 size.
     */
    CFeeRate(const CAmount& nFeePaid, int32_t virtual_bytes);

    /**
     * Return the fee in satoshis for the given vsize in vbytes.
     * If the calculated fee would have fractional satoshis, then the
     * returned fee will always be rounded up to the nearest satoshi.
     */
    CAmount GetFee(int32_t virtual_bytes) const;

    /**
     * Return the fee in satoshis for a vsize of 1000 vbytes
     */
    CAmount GetFeePerK() const { return CAmount(m_feerate.EvaluateFeeDown(1000)); }
    friend std::weak_ordering operator<=>(const CFeeRate& a, const CFeeRate& b) noexcept
    {
        return FeeRateCompare(a.m_feerate, b.m_feerate);
    }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) noexcept
    {
        return FeeRateCompare(a.m_feerate, b.m_feerate) == std::weak_ordering::equivalent;
    }
    CFeeRate& operator+=(const CFeeRate& a) {
        m_feerate = FeePerVSize(GetFeePerK() + a.GetFeePerK(), 1000);
        return *this;
    }
    std::string ToString(const FeeEstimateMode& fee_estimate_mode = FeeEstimateMode::BTC_KVB) const;
    friend CFeeRate operator*(const CFeeRate& f, int a) { return CFeeRate(a * f.m_feerate.fee, f.m_feerate.size); }
    friend CFeeRate operator*(int a, const CFeeRate& f) { return CFeeRate(a * f.m_feerate.fee, f.m_feerate.size); }

    SERIALIZE_METHODS(CFeeRate, obj) { READWRITE(obj.m_feerate.fee, obj.m_feerate.size); }
};

#endif // BITCOIN_POLICY_FEERATE_H
