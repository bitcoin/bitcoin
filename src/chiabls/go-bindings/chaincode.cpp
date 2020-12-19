// Copyright (c) 2020 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "chiabls/bls.hpp"
#include "chaincode.h"
#include "error.h"

CChainCode CChainCodeFromBytes(void *p, bool *didErr) {
    bls::ChainCode* ccPtr;
    try {
        ccPtr = new bls::ChainCode(bls::ChainCode::FromBytes(static_cast<uint8_t*>(p)));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return ccPtr;
}

void CChainCodeFree(CChainCode inPtr) {
    bls::ChainCode* cc = (bls::ChainCode*)inPtr;
    delete cc;
}

int CChainCodeSizeBytes() {
    return bls::ChainCode::CHAIN_CODE_SIZE;
}

void* CChainCodeSerialize(CChainCode inPtr) {
    bls::ChainCode* cc = (bls::ChainCode*)inPtr;

    uint8_t* buffer = static_cast<uint8_t*>(
        malloc(bls::ChainCode::CHAIN_CODE_SIZE));

    cc->Serialize(buffer);
    return static_cast<void*>(buffer);
}

bool CChainCodeIsEqual(CChainCode aPtr, CChainCode bPtr) {
    bls::ChainCode* a = (bls::ChainCode*)aPtr;
    bls::ChainCode* b = (bls::ChainCode*)bPtr;

    return *a == *b;
}
