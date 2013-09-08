/// \file       ParamGeneration.h
///
/// \brief      Parameter generation routines for Zerocoin.
///
/// \author     Ian Miers, Christina Garman and Matthew Green
/// \date       June 2013
///
/// \copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
/// \license    This project is released under the MIT license.

#ifndef PARAMGENERATION_H_
#define PARAMGENERATION_H_

namespace libzerocoin {

void CalculateParams(Params &params, Bignum N, std::string aux, uint32_t securityLevel);
void calculateGroupParamLengths(uint32_t maxPLen, uint32_t securityLevel,
                                uint32_t *pLen, uint32_t *qLen);

// Constants
#define STRING_COMMIT_GROUP         "COIN_COMMITMENT_GROUP"
#define STRING_AVC_GROUP            "ACCUMULATED_VALUE_COMMITMENT_GROUP"
#define STRING_AVC_ORDER            "ACCUMULATED_VALUE_COMMITMENT_ORDER"
#define STRING_AIC_GROUP            "ACCUMULATOR_INTERNAL_COMMITMENT_GROUP"
#define STRING_QRNCOMMIT_GROUPG     "ACCUMULATOR_QRN_COMMITMENT_GROUPG"
#define STRING_QRNCOMMIT_GROUPH     "ACCUMULATOR_QRN_COMMITMENT_GROUPH"
#define ACCUMULATOR_BASE_CONSTANT   31
#define MAX_PRIMEGEN_ATTEMPTS       10000
#define MAX_ACCUMGEN_ATTEMPTS       10000
#define MAX_GENERATOR_ATTEMPTS      10000
#define NUM_SCHNORRGEN_ATTEMPTS     10000

// Prototypes
bool                primalityTestByTrialDivision(uint32_t candidate);
uint256             calculateSeed(Bignum modulus, std::string auxString, uint32_t securityLevel, std::string groupName);
uint256             calculateGeneratorSeed(uint256 seed, uint256 pSeed, uint256 qSeed, std::string label, uint32_t index, uint32_t count);

uint256             calculateHash(uint256 input);
IntegerGroupParams  deriveIntegerGroupParams(uint256 seed, uint32_t pLen, uint32_t qLen);
IntegerGroupParams  deriveIntegerGroupFromOrder(Bignum &groupOrder);
void                calculateGroupModulusAndOrder(uint256 seed, uint32_t pLen, uint32_t qLen,
        Bignum *resultModulus, Bignum *resultGroupOrder,
        uint256 *resultPseed, uint256 *resultQseed);
Bignum              calculateGroupGenerator(uint256 seed, uint256 pSeed, uint256 qSeed, Bignum modulus,
        Bignum groupOrder, uint32_t index);
Bignum              generateRandomPrime(uint32_t primeBitLen, uint256 in_seed, uint256 *out_seed,
                                        uint32_t *prime_gen_counter);
Bignum              generateIntegerFromSeed(uint32_t numBits, uint256 seed, uint32_t *numIterations);
bool                primalityTestByTrialDivision(uint32_t candidate);

}/* namespace libzerocoin */

#endif /* PARAMGENERATION_H_ */
