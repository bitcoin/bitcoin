#ifndef BIP32CHAINCODE_H_
#define BIP32CHAINCODE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* BIP32ChainCode;

void* BIP32ChainCodeSerialize(const BIP32ChainCode cc);
bool BIP32ChainCodeIsEqual(const BIP32ChainCode cc1, const BIP32ChainCode cc2);
void BIP32ChainCodeFree(const BIP32ChainCode cc);

#ifdef __cplusplus
}
#endif
#endif  // BIP32CHAINCODE_H_