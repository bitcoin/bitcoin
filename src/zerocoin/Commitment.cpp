/**
 * @file       Commitment.cpp
 *
 * @brief      Commitment and CommitmentProof classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#include <stdlib.h>
#include "Zerocoin.h"

namespace libzerocoin {

//Commitment class
Commitment::Commitment::Commitment(const IntegerGroupParams* p,
                                   const Bignum& value): params(p), contents(value) {
	this->randomness = Bignum::randBignum(params->groupOrder);
	this->commitmentValue = (params->g.pow_mod(this->contents, params->modulus).mul_mod(
	                         params->h.pow_mod(this->randomness, params->modulus), params->modulus));
}

const Bignum& Commitment::getCommitmentValue() const {
	return this->commitmentValue;
}

const Bignum& Commitment::getRandomness() const {
	return this->randomness;
}

const Bignum& Commitment::getContents() const {
	return this->contents;
}

//CommitmentProofOfKnowledge class
CommitmentProofOfKnowledge::CommitmentProofOfKnowledge(const IntegerGroupParams* ap, const IntegerGroupParams* bp): ap(ap), bp(bp) {}

// TODO: get parameters from the commitment group
CommitmentProofOfKnowledge::CommitmentProofOfKnowledge(const IntegerGroupParams* aParams,
        const IntegerGroupParams* bParams, const Commitment& a, const Commitment& b):
	ap(aParams),bp(bParams)
{
	Bignum r1, r2, r3;

	// First: make sure that the two commitments have the
	// same contents.
	if (a.getContents() != b.getContents()) {
		throw std::invalid_argument("Both commitments must contain the same value");
	}

	// Select three random values "r1, r2, r3" in the range 0 to (2^l)-1 where l is:
	// length of challenge value + max(modulus 1, modulus 2, order 1, order 2) + margin.
	// We set "margin" to be a relatively generous  security parameter.
	//
	// We choose these large values to ensure statistical zero knowledge.
	uint32_t randomSize = COMMITMENT_EQUALITY_CHALLENGE_SIZE + COMMITMENT_EQUALITY_SECMARGIN +
	                      std::max(std::max(this->ap->modulus.bitSize(), this->bp->modulus.bitSize()),
	                               std::max(this->ap->groupOrder.bitSize(), this->bp->groupOrder.bitSize()));
	Bignum maxRange = (Bignum(2).pow(randomSize) - Bignum(1));

	r1 = Bignum::randBignum(maxRange);
	r2 = Bignum::randBignum(maxRange);
	r3 = Bignum::randBignum(maxRange);

	// Generate two random, ephemeral commitments "T1, T2"
	// of the form:
	// T1 = g1^r1 * h1^r2 mod p1
	// T2 = g2^r1 * h2^r3 mod p2
	//
	// Where (g1, h1, p1) are from "aParams" and (g2, h2, p2) are from "bParams".
	Bignum T1 = this->ap->g.pow_mod(r1, this->ap->modulus).mul_mod((this->ap->h.pow_mod(r2, this->ap->modulus)), this->ap->modulus);
	Bignum T2 = this->bp->g.pow_mod(r1, this->bp->modulus).mul_mod((this->bp->h.pow_mod(r3, this->bp->modulus)), this->bp->modulus);

	// Now hash commitment "A" with commitment "B" as well as the
	// parameters and the two ephemeral commitments "T1, T2" we just generated
	this->challenge = calculateChallenge(a.getCommitmentValue(), b.getCommitmentValue(), T1, T2);

	// Let "m" be the contents of the commitments "A, B". We have:
	// A =  g1^m  * h1^x  mod p1
	// B =  g2^m  * h2^y  mod p2
	// T1 = g1^r1 * h1^r2 mod p1
	// T2 = g2^r1 * h2^r3 mod p2
	//
	// Now compute:
	//  S1 = r1 + (m * challenge)   -- note, not modular arithmetic
	//  S2 = r2 + (x * challenge)   -- note, not modular arithmetic
	//  S3 = r3 + (y * challenge)   -- note, not modular arithmetic
	this->S1 = r1 + (a.getContents() * this->challenge);
	this->S2 = r2 + (a.getRandomness() * this->challenge);
	this->S3 = r3 + (b.getRandomness() * this->challenge);

	// We're done. The proof is S1, S2, S3 and "challenge", all of which
	// are stored in member variables.
}

bool CommitmentProofOfKnowledge::Verify(const Bignum& A, const Bignum& B) const
{
	// Compute the maximum range of S1, S2, S3 and verify that the given values are
	// in a correct range. This might be an unnecessary check.
	uint32_t maxSize = 64 * (COMMITMENT_EQUALITY_CHALLENGE_SIZE + COMMITMENT_EQUALITY_SECMARGIN +
	                         std::max(std::max(this->ap->modulus.bitSize(), this->bp->modulus.bitSize()),
	                                  std::max(this->ap->groupOrder.bitSize(), this->bp->groupOrder.bitSize())));

	if ((uint32_t)this->S1.bitSize() > maxSize ||
	        (uint32_t)this->S2.bitSize() > maxSize ||
	        (uint32_t)this->S3.bitSize() > maxSize ||
	        this->S1 < Bignum(0) ||
	        this->S2 < Bignum(0) ||
	        this->S3 < Bignum(0) ||
	        this->challenge < Bignum(0) ||
	        this->challenge > (Bignum(2).pow(COMMITMENT_EQUALITY_CHALLENGE_SIZE) - Bignum(1))) {
		// Invalid inputs. Reject.
		return false;
	}

	// Compute T1 = g1^S1 * h1^S2 * inverse(A^{challenge}) mod p1
	Bignum T1 = A.pow_mod(this->challenge, ap->modulus).inverse(ap->modulus).mul_mod(
	                (ap->g.pow_mod(S1, ap->modulus).mul_mod(ap->h.pow_mod(S2, ap->modulus), ap->modulus)),
	                ap->modulus);

	// Compute T2 = g2^S1 * h2^S3 * inverse(B^{challenge}) mod p2
	Bignum T2 = B.pow_mod(this->challenge, bp->modulus).inverse(bp->modulus).mul_mod(
	                (bp->g.pow_mod(S1, bp->modulus).mul_mod(bp->h.pow_mod(S3, bp->modulus), bp->modulus)),
	                bp->modulus);

	// Hash T1 and T2 along with all of the public parameters
	Bignum computedChallenge = calculateChallenge(A, B, T1, T2);

	// Return success if the computed challenge matches the incoming challenge
	if(computedChallenge == this->challenge) {
		return true;
	}

	// Otherwise return failure
	return false;
}

const Bignum CommitmentProofOfKnowledge::calculateChallenge(const Bignum& a, const Bignum& b, const Bignum &commitOne, const Bignum &commitTwo) const {
	CHashWriter hasher(0,0);

	// Hash together the following elements:
	// * A string identifying the proof
	// * Commitment A
	// * Commitment B
	// * Ephemeral commitment T1
	// * Ephemeral commitment T2
	// * A serialized instance of the commitment A parameters
	// * A serialized instance of the commitment B parameters

	hasher << std::string(ZEROCOIN_COMMITMENT_EQUALITY_PROOF);
	hasher << commitOne;
	hasher << std::string("||");
	hasher << commitTwo;
	hasher << std::string("||");
	hasher << a;
	hasher << std::string("||");
	hasher << b;
	hasher << std::string("||");
	hasher << *(this->ap);
	hasher << std::string("||");
	hasher << *(this->bp);

	// Convert the SHA256 result into a Bignum
	// Note that if we ever change the size of the hash function we will have
	// to update COMMITMENT_EQUALITY_CHALLENGE_SIZE appropriately!
	return Bignum(hasher.GetHash());
}

} /* namespace libzerocoin */
