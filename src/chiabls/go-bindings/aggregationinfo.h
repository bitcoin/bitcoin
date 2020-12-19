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

#ifndef GO_BINDINGS_AGGREGATIONINFO_H_
#define GO_BINDINGS_AGGREGATIONINFO_H_
#include <stdbool.h>
#include "publickey.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CAggregationInfo;

int CBLSMessageHashLen();

CAggregationInfo CAggregationInfoFromMsg(CPublicKey pkPtr, void* message,
    size_t len);

CAggregationInfo CAggregationInfoFromMsgHash(CPublicKey pkPtr, void *hash);

CAggregationInfo CAggregationInfoFromVectors(void **publicKeys,
    size_t numPublicKeys, void **messageHashes, size_t numMessageHashes,
    void **exponents, size_t numExponents, size_t *sizesExponents,
    bool *didErr);

CAggregationInfo MergeAggregationInfos(void **agginfos, size_t len);

void CAggregationInfoFree(CAggregationInfo inPtr);

void CAggregationInfoRemoveEntries(CAggregationInfo inPtr, void **messages,
    size_t numMessages, void **publicKeys, size_t numPublicKeys, bool *didErr);

uint8_t* CAggregationInfoGetPubKeys(CAggregationInfo inPtr,
    size_t *retNumKeys);

uint8_t* CAggregationInfoGetMessageHashes(CAggregationInfo inPtr,
    size_t *retNumHashes);

bool CAggregationInfoEmpty(CAggregationInfo inPtr);
bool CAggregationInfoIsEqual(CAggregationInfo aPtr, CAggregationInfo bPtr);
bool CAggregationInfoIsLess(CAggregationInfo aPtr, CAggregationInfo bPtr);

size_t CAggregationInfoGetLength(CAggregationInfo inPtr);

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_AGGREGATIONINFO_H_
