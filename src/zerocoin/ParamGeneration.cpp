/// \file       ParamGeneration.cpp
///
/// \brief      Parameter manipulation routines for the Zerocoin cryptographic
///             components.
///
/// \author     Ian Miers, Christina Garman and Matthew Green
/// \date       June 2013
///
/// \copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
/// \license    This project is released under the MIT license.

#include <string>
#include "Zerocoin.h"

using namespace std;

namespace libzerocoin {

/// \brief Fill in a set of Zerocoin parameters from a modulus "N".
/// \param N                A trusted RSA modulus
/// \param aux              An optional auxiliary string used in derivation
/// \param securityLevel    A security level
///
/// \throws         ZerocoinException if the process fails
///
/// Fills in a ZC_Params data structure deterministically from
/// a trustworthy RSA modulus "N", which is provided as a Bignum.
///
/// Note: this routine makes the fundamental assumption that "N"
/// encodes a valid RSA-style modulus of the form "e1*e2" for some
/// unknown safe primes "e1" and "e2". These factors must not
/// be known to any party, or the security of Zerocoin is
/// compromised. The integer "N" must be a MINIMUM of 1023
/// in length, and 3072 bits is strongly recommended.
///

void
CalculateParams(Params &params, Bignum N, string aux, uint32_t securityLevel)
{
	params.initialized = false;
	params.accumulatorParams.initialized = false;

	// Verify that |N| is > 1023 bits.
	uint32_t NLen = N.bitSize();
	if (NLen < 1023) {
		throw ZerocoinException("Modulus must be at least 1023 bits");
	}

	// Verify that "securityLevel" is  at least 80 bits (minimum).
	if (securityLevel < 80) {
		throw ZerocoinException("Security level must be at least 80 bits.");
	}

	// Set the accumulator modulus to "N".
	params.accumulatorParams.accumulatorModulus = N;

	// Calculate the required size of the field "F_p" into which
	// we're embedding the coin commitment group. This may throw an
	// exception if the securityLevel is too large to be supported
	// by the current modulus.
	uint32_t pLen = 0;
	uint32_t qLen = 0;
	calculateGroupParamLengths(NLen - 2, securityLevel, &pLen, &qLen);

	// Calculate candidate parameters ("p", "q") for the coin commitment group
	// using a deterministic process based on "N", the "aux" string, and
	// the dedicated string "COMMITMENTGROUP".
	params.coinCommitmentGroup = deriveIntegerGroupParams(calculateSeed(N, aux, securityLevel, STRING_COMMIT_GROUP),
	                             pLen, qLen);

	// Next, we derive parameters for a second Accumulated Value commitment group.
	// This is a Schnorr group with the specific property that the order of the group
	// must be exactly equal to "q" from the commitment group. We set
	// the modulus of the new group equal to "2q+1" and test to see if this is prime.
	params.serialNumberSoKCommitmentGroup = deriveIntegerGroupFromOrder(params.coinCommitmentGroup.modulus);

	// Calculate the parameters for the internal commitment
	// using the same process.
	params.accumulatorParams.accumulatorPoKCommitmentGroup = deriveIntegerGroupParams(calculateSeed(N, aux, securityLevel, STRING_AIC_GROUP),
	        qLen + 300, qLen + 1);

	// Calculate the parameters for the accumulator QRN commitment generators. This isn't really
	// a whole group, just a pair of random generators in QR_N.
	uint32_t resultCtr;
	params.accumulatorParams.accumulatorQRNCommitmentGroup.g = generateIntegerFromSeed(NLen - 1,
	        calculateSeed(N, aux, securityLevel, STRING_QRNCOMMIT_GROUPG),
	        &resultCtr).pow_mod(Bignum(2), N);
	params.accumulatorParams.accumulatorQRNCommitmentGroup.h = generateIntegerFromSeed(NLen - 1,
	        calculateSeed(N, aux, securityLevel, STRING_QRNCOMMIT_GROUPG),
	        &resultCtr).pow_mod(Bignum(2), N);

	// Calculate the accumulator base, which we calculate as "u = C**2 mod N"
	// where C is an arbitrary value. In the unlikely case that "u = 1" we increment
	// "C" and repeat.
	Bignum constant(ACCUMULATOR_BASE_CONSTANT);
	params.accumulatorParams.accumulatorBase = Bignum(1);
	for (uint32_t count = 0; count < MAX_ACCUMGEN_ATTEMPTS && params.accumulatorParams.accumulatorBase.isOne(); count++) {
		params.accumulatorParams.accumulatorBase = constant.pow_mod(Bignum(2), params.accumulatorParams.accumulatorModulus);
	}

	// Compute the accumulator range. The upper range is the largest possible coin commitment value.
	// The lower range is sqrt(upper range) + 1. Since OpenSSL doesn't have
	// a square root function we use a slightly higher approximation.
	params.accumulatorParams.maxCoinValue = params.coinCommitmentGroup.modulus;
	params.accumulatorParams.minCoinValue = Bignum(2).pow((params.coinCommitmentGroup.modulus.bitSize() / 2) + 3);

	// If all went well, mark params as successfully initialized.
	params.accumulatorParams.initialized = true;

	// If all went well, mark params as successfully initialized.
	params.initialized = true;
}

/// \brief Format a seed string by hashing several values.
/// \param N                A Bignum
/// \param aux              An auxiliary string
/// \param securityLevel    The security level in bits
/// \param groupName        A group description string
/// \throws         ZerocoinException if the process fails
///
/// Returns the hash of the value.

uint256
calculateGeneratorSeed(uint256 seed, uint256 pSeed, uint256 qSeed, string label, uint32_t index, uint32_t count)
{
	CHashWriter hasher(0,0);
	uint256     hash;

	// Compute the hash of:
	// <modulus>||<securitylevel>||<auxString>||groupName
	hasher << seed;
	hasher << string("||");
	hasher << pSeed;
	hasher << string("||");
	hasher << qSeed;
	hasher << string("||");
	hasher << label;
	hasher << string("||");
	hasher << index;
	hasher << string("||");
	hasher << count;

	return hasher.GetHash();
}

/// \brief Format a seed string by hashing several values.
/// \param N                A Bignum
/// \param aux              An auxiliary string
/// \param securityLevel    The security level in bits
/// \param groupName        A group description string
/// \throws         ZerocoinException if the process fails
///
/// Returns the hash of the value.

uint256
calculateSeed(Bignum modulus, string auxString, uint32_t securityLevel, string groupName)
{
	CHashWriter hasher(0,0);
	uint256     hash;

	// Compute the hash of:
	// <modulus>||<securitylevel>||<auxString>||groupName
	hasher << modulus;
	hasher << string("||");
	hasher << securityLevel;
	hasher << string("||");
	hasher << auxString;
	hasher << string("||");
	hasher << groupName;

	return hasher.GetHash();
}

uint256
calculateHash(uint256 input)
{
	CHashWriter hasher(0,0);

	// Compute the hash of "input"
	hasher << input;

	return hasher.GetHash();
}

/// \brief Calculate field/group parameter sizes based on a security level.
/// \param maxPLen          Maximum size of the field (modulus "p") in bits.
/// \param securityLevel    Required security level in bits (at least 80)
/// \param pLen             Result: length of "p" in bits
/// \param qLen             Result: length of "q" in bits
/// \throws                 ZerocoinException if the process fails
///
/// Calculates the appropriate sizes of "p" and "q" for a prime-order
/// subgroup of order "q" embedded within a field "F_p". The sizes
/// are based on a 'securityLevel' provided in symmetric-equivalent
/// bits. Our choices slightly exceed the specs in FIPS 186-3:
///
/// securityLevel = 80:     pLen = 1024, qLen = 256
/// securityLevel = 112:    pLen = 2048, qLen = 256
/// securityLevel = 128:    qLen = 3072, qLen = 320
///
/// If the length of "p" exceeds the length provided in "maxPLen", or
/// if "securityLevel < 80" this routine throws an exception.

void
calculateGroupParamLengths(uint32_t maxPLen, uint32_t securityLevel,
                           uint32_t *pLen, uint32_t *qLen)
{
	*pLen = *qLen = 0;

	if (securityLevel < 80) {
		throw ZerocoinException("Security level must be at least 80 bits.");
	} else if (securityLevel == 80) {
		*qLen = 256;
		*pLen = 1024;
	} else if (securityLevel <= 112) {
		*qLen = 256;
		*pLen = 2048;
	} else if (securityLevel <= 128) {
		*qLen = 320;
		*pLen = 3072;
	} else {
		throw ZerocoinException("Security level not supported.");
	}

	if (*pLen > maxPLen) {
		throw ZerocoinException("Modulus size is too small for this security level.");
	}
}

/// \brief Deterministically compute a set of group parameters using NIST procedures.
/// \param seedStr  A byte string seeding the process.
/// \param pLen     The desired length of the modulus "p" in bits
/// \param qLen     The desired length of the order "q" in bits
/// \return         An IntegerGroupParams object
///
/// Calculates the description of a group G of prime order "q" embedded within
/// a field "F_p". The input to this routine is in arbitrary seed. It uses the
/// algorithms described in FIPS 186-3 Appendix A.1.2 to calculate
/// primes "p" and "q". It uses the procedure in Appendix A.2.3 to
/// derive two generators "g", "h".

IntegerGroupParams
deriveIntegerGroupParams(uint256 seed, uint32_t pLen, uint32_t qLen)
{
	IntegerGroupParams result;
	Bignum p;
	Bignum q;
	uint256 pSeed, qSeed;

	// Calculate "p" and "q" and "domain_parameter_seed" from the
	// "seed" buffer above, using the procedure described in NIST
	// FIPS 186-3, Appendix A.1.2.
	calculateGroupModulusAndOrder(seed, pLen, qLen, &(result.modulus),
	                              &(result.groupOrder), &pSeed, &qSeed);

	// Calculate the generators "g", "h" using the process described in
	// NIST FIPS 186-3, Appendix A.2.3. This algorithm takes ("p", "q",
	// "domain_parameter_seed", "index"). We use "index" value 1
	// to generate "g" and "index" value 2 to generate "h".
	result.g = calculateGroupGenerator(seed, pSeed, qSeed, result.modulus, result.groupOrder, 1);
	result.h = calculateGroupGenerator(seed, pSeed, qSeed, result.modulus, result.groupOrder, 2);

	// Perform some basic tests to make sure we have good parameters
	if ((uint32_t)(result.modulus.bitSize()) < pLen ||          // modulus is pLen bits long
	        (uint32_t)(result.groupOrder.bitSize()) < qLen ||       // order is qLen bits long
	        !(result.modulus.isPrime()) ||                          // modulus is prime
	        !(result.groupOrder.isPrime()) ||                       // order is prime
	        !((result.g.pow_mod(result.groupOrder, result.modulus)).isOne()) || // g^order mod modulus = 1
	        !((result.h.pow_mod(result.groupOrder, result.modulus)).isOne()) || // h^order mod modulus = 1
	        ((result.g.pow_mod(Bignum(100), result.modulus)).isOne()) ||        // g^100 mod modulus != 1
	        ((result.h.pow_mod(Bignum(100), result.modulus)).isOne()) ||        // h^100 mod modulus != 1
	        result.g == result.h ||                                 // g != h
	        result.g.isOne()) {                                     // g != 1
		// If any of the above tests fail, throw an exception
		throw ZerocoinException("Group parameters are not valid");
	}

	return result;
}

/// \brief Deterministically compute a  set of group parameters with a specified order.
/// \param groupOrder   The order of the group
/// \return         An IntegerGroupParams object
///
/// Given "q" calculates the description of a group G of prime order "q" embedded within
/// a field "F_p".

IntegerGroupParams
deriveIntegerGroupFromOrder(Bignum &groupOrder)
{
	IntegerGroupParams result;

	// Set the order to "groupOrder"
	result.groupOrder = groupOrder;

	// Try possible values for "modulus" of the form "groupOrder * 2 * i" where
	// "p" is prime and i is a counter starting at 1.
	for (uint32_t i = 1; i < NUM_SCHNORRGEN_ATTEMPTS; i++) {
		// Set modulus equal to "groupOrder * 2 * i"
		result.modulus = (result.groupOrder * Bignum(i*2)) + Bignum(1);

		// Test the result for primality
		// TODO: This is a probabilistic routine and thus not the right choice
		if (result.modulus.isPrime(256)) {

			// Success.
			//
			// Calculate the generators "g", "h" using the process described in
			// NIST FIPS 186-3, Appendix A.2.3. This algorithm takes ("p", "q",
			// "domain_parameter_seed", "index"). We use "index" value 1
			// to generate "g" and "index" value 2 to generate "h".
			uint256 seed = calculateSeed(groupOrder, "", 128, "");
			uint256 pSeed = calculateHash(seed);
			uint256 qSeed = calculateHash(pSeed);
			result.g = calculateGroupGenerator(seed, pSeed, qSeed, result.modulus, result.groupOrder, 1);
			result.h = calculateGroupGenerator(seed, pSeed, qSeed, result.modulus, result.groupOrder, 2);

			// Perform some basic tests to make sure we have good parameters
			if (!(result.modulus.isPrime()) ||                          // modulus is prime
			        !(result.groupOrder.isPrime()) ||                       // order is prime
			        !((result.g.pow_mod(result.groupOrder, result.modulus)).isOne()) || // g^order mod modulus = 1
			        !((result.h.pow_mod(result.groupOrder, result.modulus)).isOne()) || // h^order mod modulus = 1
			        ((result.g.pow_mod(Bignum(100), result.modulus)).isOne()) ||        // g^100 mod modulus != 1
			        ((result.h.pow_mod(Bignum(100), result.modulus)).isOne()) ||        // h^100 mod modulus != 1
			        result.g == result.h ||                                 // g != h
			        result.g.isOne()) {                                     // g != 1
				// If any of the above tests fail, throw an exception
				throw ZerocoinException("Group parameters are not valid");
			}

			return result;
		}
	}

	// If we reached this point group generation has failed. Throw an exception.
	throw ZerocoinException("Too many attempts to generate Schnorr group.");
}

/// \brief Deterministically compute a group description using NIST procedures.
/// \param seed                         A byte string seeding the process.
/// \param pLen                         The desired length of the modulus "p" in bits
/// \param qLen                         The desired length of the order "q" in bits
/// \param resultModulus                A value "p" describing a finite field "F_p"
/// \param resultGroupOrder             A value "q" describing the order of a subgroup
/// \param resultDomainParameterSeed    A resulting seed for use in later calculations.
///
/// Calculates the description of a group G of prime order "q" embedded within
/// a field "F_p". The input to this routine is in arbitrary seed. It uses the
/// algorithms described in FIPS 186-3 Appendix A.1.2 to calculate
/// primes "p" and "q".

void
calculateGroupModulusAndOrder(uint256 seed, uint32_t pLen, uint32_t qLen,
                              Bignum *resultModulus, Bignum *resultGroupOrder,
                              uint256 *resultPseed, uint256 *resultQseed)
{
	// Verify that the seed length is >= qLen
	if (qLen > (sizeof(seed)) * 8) {
		// TODO: The use of 256-bit seeds limits us to 256-bit group orders. We should probably change this.
		// throw ZerocoinException("Seed is too short to support the required security level.");
	}

#ifdef ZEROCOIN_DEBUG
	cout << "calculateGroupModulusAndOrder: pLen = " << pLen << endl;
#endif

	// Generate a random prime for the group order.
	// This may throw an exception, which we'll pass upwards.
	// Result is the value "resultGroupOrder", "qseed" and "qgen_counter".
	uint256     qseed;
	uint32_t    qgen_counter;
	*resultGroupOrder = generateRandomPrime(qLen, seed, &qseed, &qgen_counter);

	// Using ⎡pLen / 2 + 1⎤ as the length and qseed as the input_seed, use the random prime
	// routine to obtain p0 , pseed, and pgen_counter. We pass exceptions upward.
	uint32_t    p0len = ceil((pLen / 2.0) + 1);
	uint256     pseed;
	uint32_t    pgen_counter;
	Bignum p0 = generateRandomPrime(p0len, qseed, &pseed, &pgen_counter);

	// Set x = 0, old_counter = pgen_counter
	uint32_t    old_counter = pgen_counter;

	// Generate a random integer "x" of pLen bits
	uint32_t iterations;
	Bignum x = generateIntegerFromSeed(pLen, pseed, &iterations);
	pseed += (iterations + 1);

	// Set x = 2^{pLen−1} + (x mod 2^{pLen–1}).
	Bignum powerOfTwo = Bignum(2).pow(pLen-1);
	x = powerOfTwo + (x % powerOfTwo);

	// t = ⎡x / (2 * resultGroupOrder * p0)⎤.
	// TODO: we don't have a ceiling function
	Bignum t = x / (Bignum(2) * (*resultGroupOrder) * p0);

	// Now loop until we find a valid prime "p" or we fail due to
	// pgen_counter exceeding ((4*pLen) + old_counter).
	for ( ; pgen_counter <= ((4*pLen) + old_counter) ; pgen_counter++) {
		// If (2 * t * resultGroupOrder * p0 + 1) > 2^{pLen}, then
		// t = ⎡2^{pLen−1} / (2 * resultGroupOrder * p0)⎤.
		powerOfTwo = Bignum(2).pow(pLen);
		Bignum prod = (Bignum(2) * t * (*resultGroupOrder) * p0) + Bignum(1);
		if (prod > powerOfTwo) {
			// TODO: implement a ceil function
			t = Bignum(2).pow(pLen-1) / (Bignum(2) * (*resultGroupOrder) * p0);
		}

		// Compute a candidate prime resultModulus = 2tqp0 + 1.
		*resultModulus = (Bignum(2) * t * (*resultGroupOrder) * p0) + Bignum(1);

		// Verify that resultModulus is prime. First generate a pseudorandom integer "a".
		Bignum a = generateIntegerFromSeed(pLen, pseed, &iterations);
		pseed += iterations + 1;

		// Set a = 2 + (a mod (resultModulus–3)).
		a = Bignum(2) + (a % ((*resultModulus) - Bignum(3)));

		// Set z = a^{2 * t * resultGroupOrder} mod resultModulus
		Bignum z = a.pow_mod(Bignum(2) * t * (*resultGroupOrder), (*resultModulus));

		// If GCD(z–1, resultModulus) == 1 AND (z^{p0} mod resultModulus == 1)
		// then we have found our result. Return.
		if ((resultModulus->gcd(z - Bignum(1))).isOne() &&
		        (z.pow_mod(p0, (*resultModulus))).isOne()) {
			// Success! Return the seeds and primes.
			*resultPseed = pseed;
			*resultQseed = qseed;
			return;
		}

		// This prime did not work out. Increment "t" and try again.
		t = t + Bignum(1);
	} // loop continues until pgen_counter exceeds a limit

	// We reach this point only if we exceeded our maximum iteration count.
	// Throw an exception.
	throw ZerocoinException("Unable to generate a prime modulus for the group");
}

/// \brief Deterministically compute a generator for a given group.
/// \param seed                         A first seed for the process.
/// \param pSeed                        A second seed for the process.
/// \param qSeed                        A third seed for the process.
/// \param modulus                      Proposed prime modulus for the field.
/// \param groupOrder                   Proposed order of the group.
/// \param index                        Index value, selects which generator you're building.
/// \return                             The resulting generator.
/// \throws                             A ZerocoinException if error.
///
/// Generates a random group generator deterministically as a function of (seed,pSeed,qSeed)
/// Uses the algorithm described in FIPS 186-3 Appendix A.2.3.

Bignum
calculateGroupGenerator(uint256 seed, uint256 pSeed, uint256 qSeed, Bignum modulus, Bignum groupOrder, uint32_t index)
{
	Bignum result;

	// Verify that 0 <= index < 256
	if (index > 255) {
		throw ZerocoinException("Invalid index for group generation");
	}

	// Compute e = (modulus - 1) / groupOrder
	Bignum e = (modulus - Bignum(1)) / groupOrder;

	// Loop until we find a generator
	for (uint32_t count = 1; count < MAX_GENERATOR_ATTEMPTS; count++) {
		// hash = Hash(seed || pSeed || qSeed || “ggen” || index || count
		uint256 hash = calculateGeneratorSeed(seed, pSeed, qSeed, "ggen", index, count);
		Bignum W(hash);

		// Compute result = W^e mod p
		result = W.pow_mod(e, modulus);

		// If result > 1, we have a generator
		if (result > 1) {
			return result;
		}
	}

	// We only get here if we failed to find a generator
	throw ZerocoinException("Unable to find a generator, too many attempts");
}

/// \brief Deterministically compute a random prime number.
/// \param primeBitLen                  Desired bit length of the prime.
/// \param in_seed                      Input seed for the process.
/// \param out_seed                     Result: output seed from the process.
/// \param prime_gen_counter            Result: number of iterations required.
/// \return                             The resulting prime number.
/// \throws                             A ZerocoinException if error.
///
/// Generates a random prime number of primeBitLen bits from a given input
/// seed. Uses the Shawe-Taylor algorithm as described in FIPS 186-3
/// Appendix C.6. This is a recursive function.

Bignum
generateRandomPrime(uint32_t primeBitLen, uint256 in_seed, uint256 *out_seed,
                    uint32_t *prime_gen_counter)
{
	// Verify that primeBitLen is not too small
	if (primeBitLen < 2) {
		throw ZerocoinException("Prime length is too short");
	}

	// If primeBitLen < 33 bits, perform the base case.
	if (primeBitLen < 33) {
		Bignum result(0);

		// Set prime_seed = in_seed, prime_gen_counter = 0.
		uint256     prime_seed = in_seed;
		(*prime_gen_counter) = 0;

		// Loop up to "4 * primeBitLen" iterations.
		while ((*prime_gen_counter) < (4 * primeBitLen)) {

			// Generate a pseudorandom integer "c" of length primeBitLength bits
			uint32_t iteration_count;
			Bignum c = generateIntegerFromSeed(primeBitLen, prime_seed, &iteration_count);
#ifdef ZEROCOIN_DEBUG
			cout << "generateRandomPrime: primeBitLen = " << primeBitLen << endl;
			cout << "Generated c = " << c << endl;
#endif

			prime_seed += (iteration_count + 1);
			(*prime_gen_counter)++;

			// Set "intc" to be the least odd integer >= "c" we just generated
			uint32_t intc = c.getulong();
			intc = (2 * floor(intc / 2.0)) + 1;
#ifdef ZEROCOIN_DEBUG
			cout << "Should be odd. c = " << intc << endl;
			cout << "The big num is: c = " << c << endl;
#endif

			// Perform trial division on this (relatively small) integer to determine if "intc"
			// is prime. If so, return success.
			if (primalityTestByTrialDivision(intc)) {
				// Return "intc" converted back into a Bignum and "prime_seed". We also updated
				// the variable "prime_gen_counter" in previous statements.
				result = intc;
				*out_seed = prime_seed;

				// Success
				return result;
			}
		} // while()

		// If we reached this point there was an error finding a candidate prime
		// so throw an exception.
		throw ZerocoinException("Unable to find prime in Shawe-Taylor algorithm");

		// END OF BASE CASE
	}
	// If primeBitLen >= 33 bits, perform the recursive case.
	else {
		// Recurse to find a new random prime of roughly half the size
		uint32_t newLength = ceil((double)primeBitLen / 2.0) + 1;
		Bignum c0 = generateRandomPrime(newLength, in_seed, out_seed, prime_gen_counter);

		// Generate a random integer "x" of primeBitLen bits using the output
		// of the previous call.
		uint32_t numIterations;
		Bignum x = generateIntegerFromSeed(primeBitLen, *out_seed, &numIterations);
		(*out_seed) += numIterations + 1;

		// Compute "t" = ⎡x / (2 * c0⎤
		// TODO no Ceiling call
		Bignum t = x / (Bignum(2) * c0);

		// Repeat the following procedure until we find a prime (or time out)
		for (uint32_t testNum = 0; testNum < MAX_PRIMEGEN_ATTEMPTS; testNum++) {

			// If ((2 * t * c0) + 1 > 2^{primeBitLen}),
			// then t = ⎡2^{primeBitLen} – 1 / (2 * c0)⎤.
			if ((Bignum(2) * t * c0) > (Bignum(2).pow(Bignum(primeBitLen)))) {
				t = ((Bignum(2).pow(Bignum(primeBitLen))) - Bignum(1)) / (Bignum(2) * c0);
			}

			// Set c = (2 * t * c0) + 1
			Bignum c = (Bignum(2) * t * c0) + Bignum(1);

			// Increment prime_gen_counter
			(*prime_gen_counter)++;

			// Test "c" for primality as follows:
			// 1. First pick an integer "a" in between 2 and (c - 2)
			Bignum a = generateIntegerFromSeed(c.bitSize(), (*out_seed), &numIterations);
			a = Bignum(2) + (a % (c - Bignum(3)));
			(*out_seed) += (numIterations + 1);

			// 2. Compute "z" = a^{2*t} mod c
			Bignum z = a.pow_mod(Bignum(2) * t, c);

			// 3. Check if "c" is prime.
			//    Specifically, verify that gcd((z-1), c) == 1 AND (z^c0 mod c) == 1
			// If so we return "c" as our result.
			if (c.gcd(z - Bignum(1)).isOne() && z.pow_mod(c0, c).isOne()) {
				// Return "c", out_seed and prime_gen_counter
				// (the latter two of which were already updated)
				return c;
			}

			// 4. If the test did not succeed, increment "t" and loop
			t = t + Bignum(1);
		} // end of test loop
	}

	// We only reach this point if the test loop has iterated MAX_PRIMEGEN_ATTEMPTS
	// and failed to identify a valid prime. Throw an exception.
	throw ZerocoinException("Unable to generate random prime (too many tests)");
}

Bignum
generateIntegerFromSeed(uint32_t numBits, uint256 seed, uint32_t *numIterations)
{
	Bignum      result(0);
	uint32_t    iterations = ceil((double)numBits / (double)HASH_OUTPUT_BITS);

#ifdef ZEROCOIN_DEBUG
	cout << "numBits = " << numBits << endl;
	cout << "iterations = " << iterations << endl;
#endif

	// Loop "iterations" times filling up the value "result" with random bits
	for (uint32_t count = 0; count < iterations; count++) {
		// result += ( H(pseed + count) * 2^{count * p0len} )
		result += Bignum(calculateHash(seed + count)) * Bignum(2).pow(count * HASH_OUTPUT_BITS);
	}

	result = Bignum(2).pow(numBits - 1) + (result % (Bignum(2).pow(numBits - 1)));

	// Return the number of iterations and the result
	*numIterations = iterations;
	return result;
}

/// \brief Determines whether a uint32_t is a prime through trial division.
/// \param candidate       Candidate to test.
/// \return                true if the value is prime, false otherwise
///
/// Performs trial division to determine whether a uint32_t is prime.

bool
primalityTestByTrialDivision(uint32_t candidate)
{
	// TODO: HACK HACK WRONG WRONG
	Bignum canBignum(candidate);

	return canBignum.isPrime();
}

} // namespace libzerocoin
