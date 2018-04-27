/**
 * @file       Commitment.h
 *
 * @brief      Commitment and CommitmentProof classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#ifndef COMMITMENT_H_
#define COMMITMENT_H_

#include "Params.h"
#include "../serialize.h"

// We use a SHA256 hash for our PoK challenges. Update the following
// if we ever change hash functions.
#define COMMITMENT_EQUALITY_CHALLENGE_SIZE  256

// A 512-bit security parameter for the statistical ZK PoK.
#define COMMITMENT_EQUALITY_SECMARGIN       512

namespace libzerocoin {

/**
 * A commitment, complete with contents and opening randomness.
 * These should remain secret. Publish only the commitment value.
 */
class Commitment {
public:
	/**Generates a Pedersen commitment to the given value.
	 *
	 * @param p the group parameters for the coin
	 * @param value the value to commit to
	 */
	Commitment(const IntegerGroupParams* p, const Bignum& value);
	const Bignum& getCommitmentValue() const;
	const Bignum& getRandomness() const;
	const Bignum& getContents() const;
private:
	const IntegerGroupParams *params;
	Bignum commitmentValue;
	Bignum randomness;
	const Bignum contents;
	IMPLEMENT_SERIALIZE
	(
	    READWRITE(commitmentValue);
	    READWRITE(randomness);
	    READWRITE(contents);
	)
};

/**Proof that two commitments open to the same value.
 *
 */
class CommitmentProofOfKnowledge {
public:
	CommitmentProofOfKnowledge(const IntegerGroupParams* ap, const IntegerGroupParams* bp);
	/** Generates a proof that two commitments, a and b, open to the same value.
	 *
	 * @param ap the IntegerGroup for commitment a
	 * @param bp the IntegerGroup for commitment b
	 * @param a the first commitment
	 * @param b the second commitment
	 */
	CommitmentProofOfKnowledge(const IntegerGroupParams* aParams, const IntegerGroupParams* bParams, const Commitment& a, const Commitment& b);
	//FIXME: is it best practice that this is here?
	template<typename Stream>
	CommitmentProofOfKnowledge(const IntegerGroupParams* aParams,
	                           const IntegerGroupParams* bParams, Stream& strm): ap(aParams), bp(bParams)
	{
		strm >> *this;
	}

	const Bignum calculateChallenge(const Bignum& a, const Bignum& b, const Bignum &commitOne, const Bignum &commitTwo) const;

	/**Verifies the proof
	 *
	 * @return true if the proof is valid.
	 */
	/**Verifies the proof of equality of the two commitments
	 *
	 * @param A value of commitment one
	 * @param B value of commitment two
	 * @return
	 */
	bool Verify(const Bignum& A, const Bignum& B) const;
	IMPLEMENT_SERIALIZE
	(
	    READWRITE(S1);
	    READWRITE(S2);
	    READWRITE(S3);
	    READWRITE(challenge);
	)
private:
	const IntegerGroupParams *ap, *bp;

	Bignum S1, S2, S3, challenge;
};

} /* namespace libzerocoin */
#endif /* COMMITMENT_H_ */
