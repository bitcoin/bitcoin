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

#ifndef ELEMENTS_H_
#define ELEMENTS_H_
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void* G1Element;
typedef void* G2Element;
typedef void* PrivateKey;

// G1Element
int G1ElementSize();
G1Element G1ElementFromBytes(const void* data, const bool legacy, bool* didErr);
G1Element G1ElementGenerator();
bool G1ElementIsValid(const G1Element el);
uint32_t G1ElementGetFingerprint(const G1Element el, const bool legacy);
bool G1ElementIsEqual(const G1Element el1, const G1Element el2);
G1Element G1ElementAdd(const G1Element el1, const G1Element el2);
G1Element G1ElementMul(const G1Element el, const PrivateKey sk);
G1Element G1ElementNegate(const G1Element el);
G1Element G1ElementCopy(const G1Element el);
void* G1ElementSerialize(const G1Element el, const bool legacy);
void G1ElementFree(const G1Element el);

// G2Element
int G2ElementSize();
G2Element G2ElementFromBytes(const void* data, const bool legacy, bool* didErr);
G2Element G2ElementGenerator();
bool G2ElementIsValid(const G2Element el);
bool G2ElementIsEqual(const G2Element el1, const G2Element el2);
G2Element G2ElementAdd(const G2Element el1, const G2Element el2);
G2Element G2ElementMul(const G2Element el, const PrivateKey sk);
G2Element G2ElementNegate(const G2Element el);
G2Element G2ElementCopy(const G2Element el);
void* G2ElementSerialize(const G2Element el, const bool legacy);
void G2ElementFree(const G2Element el);

#ifdef __cplusplus
}
#endif
#endif // ELEMENTS_H_
