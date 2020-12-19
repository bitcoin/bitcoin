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

#include "blschia.h"
#include <string>
#include "chiabls/bls.hpp"
#include "error.h"

std::string gErrMsg;

void SecFree(void *p) {
    bls::Util::SecFree(p);
}

void** AllocPtrArray(size_t len) {
    // caller to free
    return static_cast<void**>(malloc(sizeof(void*) * len));
}

void SetPtrArray(void **arrPtr, void *elemPtr, int index) {
    arrPtr[index] = elemPtr;
}

void FreePtrArray(void **inPtr) {
    free(inPtr);
}

void* GetPtrAtIndex(void **arrPtr, int index) {
    return arrPtr[index];
}

void* GetAddressAtIndex(uint8_t *ptr, int index) {
    return static_cast<void*>(&ptr[index]);
}

const char* GetLastErrorMsg() {
    return gErrMsg.c_str();
}
