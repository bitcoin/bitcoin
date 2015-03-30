#ifndef OMNICORE_ENCODING_H
#define OMNICORE_ENCODING_H

class CPubKey;
class CTxOut;

#include <string>
#include <vector>
#include "script/script.h"
#include "script/standard.h"

bool OmniCore_Encode_ClassB(const std::string& senderAddress, const CPubKey& redeemingPubKey, const std::vector<unsigned char>& vecPayload, std::vector<std::pair <CScript,int64_t> >& vecOutputs);
bool OmniCore_Encode_ClassC(const std::vector<unsigned char>& vecPayload, std::vector<std::pair <CScript,int64_t> >& vecOutputs);

#endif // OMNICORE_ENCODING_H
