/**
 * @file       CoinSpend.cpp
 *
 * @brief      CoinSpend class for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#include "Zerocoin.h"

namespace libzerocoin {

CoinSpend::CoinSpend(const Params* p, const PrivateCoin& coin,
                     Accumulator& a, const AccumulatorWitness& witness, const SpendMetaData& m):
	params(p),
	denomination(coin.getPublicCoin().getDenomination()),
	coinSerialNumber((coin.getSerialNumber())),
	accumulatorPoK(&p->accumulatorParams),
	serialNumberSoK(p),
	commitmentPoK(&p->serialNumberSoKCommitmentGroup, &p->accumulatorParams.accumulatorPoKCommitmentGroup) {

	// Sanity check: let's verify that the Witness is valid with respect to
	// the coin and Accumulator provided.
	if (!(witness.VerifyWitness(a, coin.getPublicCoin()))) {
		throw std::invalid_argument("Accumulator witness does not verify");
	}

	// 1: Generate two separate commitments to the public coin (C), each under
	// a different set of public parameters. We do this because the RSA accumulator
	// has specific requirements for the commitment parameters that are not
	// compatible with the group we use for the serial number proof.
	// Specifically, our serial number proof requires the order of the commitment group
	// to be the same as the modulus of the upper group. The Accumulator proof requires a
	// group with a significantly larger order.
	const Commitment fullCommitmentToCoinUnderSerialParams(&p->serialNumberSoKCommitmentGroup, coin.getPublicCoin().getValue());
	this->serialCommitmentToCoinValue = fullCommitmentToCoinUnderSerialParams.getCommitmentValue();

	const Commitment fullCommitmentToCoinUnderAccParams(&p->accumulatorParams.accumulatorPoKCommitmentGroup, coin.getPublicCoin().getValue());
	this->accCommitmentToCoinValue = fullCommitmentToCoinUnderAccParams.getCommitmentValue();

	// 2. Generate a ZK proof that the two commitments contain the same public coin.
	this->commitmentPoK = CommitmentProofOfKnowledge(&p->serialNumberSoKCommitmentGroup, &p->accumulatorParams.accumulatorPoKCommitmentGroup, fullCommitmentToCoinUnderSerialParams, fullCommitmentToCoinUnderAccParams);

	// Now generate the two core ZK proofs:
	// 3. Proves that the committed public coin is in the Accumulator (PoK of "witness")
	this->accumulatorPoK = AccumulatorProofOfKnowledge(&p->accumulatorParams, fullCommitmentToCoinUnderAccParams, witness, a);

	// 4. Proves that the coin is correct w.r.t. serial number and hidden coin secret
	// (This proof is bound to the coin 'metadata', i.e., transaction hash)
	this->serialNumberSoK = SerialNumberSignatureOfKnowledge(p, coin, fullCommitmentToCoinUnderSerialParams, signatureHash(m));
}

const Bignum&
CoinSpend::getCoinSerialNumber() {
	return this->coinSerialNumber;
}

const CoinDenomination
CoinSpend::getDenomination() {
	return static_cast<CoinDenomination>(this->denomination);
}

bool
CoinSpend::Verify(const Accumulator& a, const SpendMetaData &m) const {
	// Verify both of the sub-proofs using the given meta-data
	return  (a.getDenomination() == this->denomination)
	        && commitmentPoK.Verify(serialCommitmentToCoinValue, accCommitmentToCoinValue)
	        && accumulatorPoK.Verify(a, accCommitmentToCoinValue)
	        && serialNumberSoK.Verify(coinSerialNumber, serialCommitmentToCoinValue, signatureHash(m));
}

const uint256 CoinSpend::signatureHash(const SpendMetaData &m) const {
	CHashWriter h(0,0);
	h << m << serialCommitmentToCoinValue << accCommitmentToCoinValue << commitmentPoK << accumulatorPoK;
	return h.GetHash();
}

} /* namespace libzerocoin */
