/**
 * @file       AccumulatorProofOfKnowledge.h
 *
 * @brief      AccumulatorProofOfKnowledge class for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#ifndef ACCUMULATEPROOF_H_
#define ACCUMULATEPROOF_H_

namespace libzerocoin {

/**A prove that a value insde the commitment commitmentToCoin is in an accumulator a.
 *
 */
class AccumulatorProofOfKnowledge {
public:
	AccumulatorProofOfKnowledge(const AccumulatorAndProofParams* p);

	/** Generates a proof that a commitment to a coin c was accumulated
	 * @param p  Cryptographic parameters
	 * @param commitmentToCoin commitment containing the coin we want to prove is accumulated
	 * @param witness The witness to the accumulation of the coin
	 * @param a
	 */
	AccumulatorProofOfKnowledge(const AccumulatorAndProofParams* p, const Commitment& commitmentToCoin, const AccumulatorWitness& witness, Accumulator& a);
	/** Verifies that  a commitment c is accumulated in accumulated a
	 */
	bool Verify(const Accumulator& a,const Bignum& valueOfCommitmentToCoin) const;

	IMPLEMENT_SERIALIZE
	(
	    READWRITE(C_e);
	    READWRITE(C_u);
	    READWRITE(C_r);
	    READWRITE(st_1);
	    READWRITE(st_2);
	    READWRITE(st_3);
	    READWRITE(t_1);
	    READWRITE(t_2);
	    READWRITE(t_3);
	    READWRITE(t_4);
	    READWRITE(s_alpha);
	    READWRITE(s_beta);
	    READWRITE(s_zeta);
	    READWRITE(s_sigma);
	    READWRITE(s_eta);
	    READWRITE(s_epsilon);
	    READWRITE(s_delta);
	    READWRITE(s_xi);
	    READWRITE(s_phi);
	    READWRITE(s_gamma);
	    READWRITE(s_psi);
	)
private:
	const AccumulatorAndProofParams* params;

	/* Return values for proof */
	Bignum C_e;
	Bignum C_u;
	Bignum C_r;

	Bignum st_1;
	Bignum st_2;
	Bignum st_3;

	Bignum t_1;
	Bignum t_2;
	Bignum t_3;
	Bignum t_4;

	Bignum s_alpha;
	Bignum s_beta;
	Bignum s_zeta;
	Bignum s_sigma;
	Bignum s_eta;
	Bignum s_epsilon;
	Bignum s_delta;
	Bignum s_xi;
	Bignum s_phi;
	Bignum s_gamma;
	Bignum s_psi;
};

} /* namespace libzerocoin */
#endif /* ACCUMULATEPROOF_H_ */
