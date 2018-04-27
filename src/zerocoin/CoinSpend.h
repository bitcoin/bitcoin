/**
 * @file       CoinSpend.h
 *
 * @brief      CoinSpend class for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#ifndef COINSPEND_H_
#define COINSPEND_H_

#include "Params.h"
#include "Coin.h"
#include "Commitment.h"
#include "../bignum.h"
#include "Accumulator.h"
#include "AccumulatorProofOfKnowledge.h"
#include "SerialNumberSignatureOfKnowledge.h"
#include "SpendMetaData.h"
#include "../serialize.h"

namespace libzerocoin {

/** The complete proof needed to spend a zerocoin.
 * Composes together a proof that a coin is accumulated
 * and that it has a given serial number.
 */
class CoinSpend {
public:
	template<typename Stream>
	CoinSpend(const Params* p,  Stream& strm):denomination(ZQ_PEDERSEN),
		accumulatorPoK(&p->accumulatorParams),
		serialNumberSoK(p),
		commitmentPoK(&p->serialNumberSoKCommitmentGroup, &p->accumulatorParams.accumulatorPoKCommitmentGroup) {
		strm >> *this;
	}
	/**Generates a proof spending a zerocoin.
	 *
	 * To use this, provide an unspent PrivateCoin, the latest Accumulator
	 * (e.g from the most recent Bitcoin block) containing the public part
	 * of the coin, a witness to that, and whatever medeta data is needed.
	 *
	 * Once constructed, this proof can be serialized and sent.
	 * It is validated simply be calling validate.
	 * @warning Validation only checks that the proof is correct
	 * @warning for the specified values in this class. These values must be validated
	 *  Clients ought to check that
	 * 1) params is the right params
	 * 2) the accumulator actually is in some block
	 * 3) that the serial number is unspent
	 * 4) that the transaction
	 *
	 * @param p cryptographic parameters
	 * @param coin The coin to be spend
	 * @param a The current accumulator containing the coin
	 * @param witness The witness showing that the accumulator contains the coin
	 * @param m arbitrary meta data related to the spend that might be needed by Bitcoin
	 * 			(i.e. the transaction hash)
	 * @throw ZerocoinException if the process fails
	 */
	CoinSpend(const Params* p, const PrivateCoin& coin, Accumulator& a, const AccumulatorWitness& witness, const SpendMetaData& m);

	/** Returns the serial number of the coin spend by this proof.
	 *
	 * @return the coin's serial number
	 */
	const Bignum& getCoinSerialNumber();

	/**Gets the denomination of the coin spent in this proof.
	 *
	 * @return the denomination
	 */
	const CoinDenomination getDenomination();

	bool Verify(const Accumulator& a, const SpendMetaData &metaData) const;

	IMPLEMENT_SERIALIZE
	(
	    READWRITE(denomination);
	    READWRITE(accCommitmentToCoinValue);
	    READWRITE(serialCommitmentToCoinValue);
	    READWRITE(coinSerialNumber);
	    READWRITE(accumulatorPoK);
	    READWRITE(serialNumberSoK);
	    READWRITE(commitmentPoK);
	)

private:
	const Params *params;
	const uint256 signatureHash(const SpendMetaData &m) const;
	// Denomination is stored as an INT because storing
	// and enum raises amigiuities in the serialize code //FIXME if possible
	int denomination;
	Bignum accCommitmentToCoinValue;
	Bignum serialCommitmentToCoinValue;
	Bignum coinSerialNumber;
	AccumulatorProofOfKnowledge accumulatorPoK;
	SerialNumberSignatureOfKnowledge serialNumberSoK;
	CommitmentProofOfKnowledge commitmentPoK;
};

} /* namespace libzerocoin */
#endif /* COINSPEND_H_ */
