// Copyright (c) 2021 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GO_BINDINGS_ELEMENTS_H_
#define GO_BINDINGS_ELEMENTS_H_
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CG1Element;
typedef void* CG2Element;
typedef void* CPrivateKey;

// G1Element
int CG1ElementSize();
CG1Element CG1ElementFromBytes(const void* data, bool* didErr);
CG1Element CG1ElementGenerator();
bool CG1ElementIsValid(const CG1Element el);
uint32_t CG1ElementGetFingerprint(const CG1Element el);
bool CG1ElementIsEqual(const CG1Element el1, const CG1Element el2);
CG1Element CG1ElementAdd(const CG1Element el1, const CG1Element el2);
CG1Element CG1ElementMul(const CG1Element el, const CPrivateKey sk);
CG1Element CG1ElementNegate(const CG1Element el);
void* CG1ElementSerialize(const CG1Element el);
void CG1ElementFree(const CG1Element el);

// G2Element
int CG2ElementSize();
CG2Element CG2ElementFromBytes(const void* data, bool* didErr);
CG2Element CG2ElementGenerator();
bool CG2ElementIsValid(const CG2Element el);
bool CG2ElementIsEqual(const CG2Element el1, const CG2Element el2);
CG2Element CG2ElementAdd(const CG2Element el1, const CG2Element el2);
CG2Element CG2ElementMul(const CG2Element el, const CPrivateKey sk);
CG2Element CG2ElementNegate(const CG2Element el);
void* CG2ElementSerialize(const CG2Element el);
void CG2ElementFree(const CG2Element el);

#ifdef __cplusplus
}
#endif
#endif // GO_BINDINGS_ELEMENTS_H_
