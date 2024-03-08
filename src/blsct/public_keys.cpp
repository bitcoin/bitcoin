// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <blsct/common.h>
#include <blsct/public_keys.h>
#include <iterator>
#include <tinyformat.h>

namespace blsct {

PublicKey PublicKeys::Aggregate() const
{
    if (m_pks.size() == 0) throw std::runtime_error(strprintf("%s: Vector of public keys is empty", __func__));

    auto retPoint = PublicKey::Point();
    bool isZero = true;

    for (auto& pk : m_pks) {
        if (!pk.fValid)
            throw std::runtime_error(strprintf("%s: Vector of public keys has an invalid element", __func__));

        retPoint = isZero ? pk.GetG1Point() : retPoint + pk.GetG1Point();
        isZero = false;
    }

    return PublicKey(retPoint);
}

bool PublicKeys::VerifyBalanceBatch(const Signature& sig) const
{
    auto aggr_pk = PublicKeys(m_pks).Aggregate();
    return aggr_pk.CoreVerify(Common::BLSCTBALANCE, sig);
}

bool PublicKeys::CoreAggregateVerify(const std::vector<PublicKey::Message>& msgs, const Signature& sig) const
{
    assert(m_pks.size() == msgs.size());

    std::vector<blsPublicKey> bls_pks;
    std::transform(m_pks.begin(), m_pks.end(), std::back_inserter(bls_pks), [](const auto& pk) {
        return pk.ToBlsPublicKey();
    });

    // find the largest message size
    auto bls_msg_size = std::max_element(msgs.begin(), msgs.end(), [](const auto& a, const auto& b) {
                            return a.size() < b.size();
                        })->size();
    const size_t n = m_pks.size();

    // copy all msgs to a vector of message buffers of the largest message size
    std::vector<uint8_t> bls_msgs(bls_msg_size * n);
    for (size_t i = 0; i < n; ++i) {
        uint8_t* msg_beg = &bls_msgs[i * bls_msg_size];
        std::memset(msg_beg, 0, bls_msg_size);
        auto msg = msgs[i];
        std::memcpy(msg_beg, &msg[0], msg.size());
    }

    auto res = blsAggregateVerifyNoCheck(
        &sig.m_data,
        &bls_pks[0],
        &bls_msgs[0],
        bls_msg_size,
        n);
    return res == 1;
}

bool PublicKeys::VerifyBatch(const std::vector<PublicKey::Message>& msgs, const Signature& sig, const bool& fVerifyTx) const
{
    if (m_pks.size() != msgs.size() || m_pks.size() == 0) {
        throw std::runtime_error(std::string(__func__) + strprintf(
            "Expected the same positive numbers of public keys and messages, but got: %ld public keys and %ld messages", m_pks.size(), msgs.size()));
    }
    std::vector<std::vector<uint8_t>> aug_msgs;
    auto msg = msgs.begin();
    for (auto pk = m_pks.begin(), end = m_pks.end(); pk != end; ++pk, ++msg) {
        if (*msg == blsct::Common::BLSCTBALANCE && fVerifyTx) {
            aug_msgs.push_back(*msg);
        } else {
            aug_msgs.push_back(pk->AugmentMessage(*msg));
        }
    }
    return CoreAggregateVerify(aug_msgs, sig);
}

} // namespace blsct
