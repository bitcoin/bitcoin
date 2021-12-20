/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/


#include <new>

#define MINISKETCH_BUILD
#ifdef _MINISKETCH_H_
#  error "minisketch.h cannot be included before minisketch.cpp"
#endif
#include "../include/minisketch.h"

#include "false_positives.h"
#include "fielddefines.h"
#include "sketch.h"

#ifdef HAVE_CLMUL
#  ifdef _MSC_VER
#    include <intrin.h>
#  else
#    include <cpuid.h>
#  endif
#endif

Sketch* ConstructGeneric1Byte(int bits, int implementation);
Sketch* ConstructGeneric2Bytes(int bits, int implementation);
Sketch* ConstructGeneric3Bytes(int bits, int implementation);
Sketch* ConstructGeneric4Bytes(int bits, int implementation);
Sketch* ConstructGeneric5Bytes(int bits, int implementation);
Sketch* ConstructGeneric6Bytes(int bits, int implementation);
Sketch* ConstructGeneric7Bytes(int bits, int implementation);
Sketch* ConstructGeneric8Bytes(int bits, int implementation);

#ifdef HAVE_CLMUL
Sketch* ConstructClMul1Byte(int bits, int implementation);
Sketch* ConstructClMul2Bytes(int bits, int implementation);
Sketch* ConstructClMul3Bytes(int bits, int implementation);
Sketch* ConstructClMul4Bytes(int bits, int implementation);
Sketch* ConstructClMul5Bytes(int bits, int implementation);
Sketch* ConstructClMul6Bytes(int bits, int implementation);
Sketch* ConstructClMul7Bytes(int bits, int implementation);
Sketch* ConstructClMul8Bytes(int bits, int implementation);
Sketch* ConstructClMulTri1Byte(int bits, int implementation);
Sketch* ConstructClMulTri2Bytes(int bits, int implementation);
Sketch* ConstructClMulTri3Bytes(int bits, int implementation);
Sketch* ConstructClMulTri4Bytes(int bits, int implementation);
Sketch* ConstructClMulTri5Bytes(int bits, int implementation);
Sketch* ConstructClMulTri6Bytes(int bits, int implementation);
Sketch* ConstructClMulTri7Bytes(int bits, int implementation);
Sketch* ConstructClMulTri8Bytes(int bits, int implementation);
#endif

namespace {

enum class FieldImpl {
    GENERIC = 0,
#ifdef HAVE_CLMUL
    CLMUL,
    CLMUL_TRI,
#endif
};

static inline bool EnableClmul()
{
#ifdef HAVE_CLMUL
#ifdef _MSC_VER
    int regs[4];
    __cpuid(regs, 1);
    return (regs[2] & 0x2);
#else
    uint32_t eax, ebx, ecx, edx;
    return (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx & 0x2));
#endif
#else
    return false;
#endif
}

Sketch* Construct(int bits, int impl)
{
    switch (FieldImpl(impl)) {
    case FieldImpl::GENERIC:
        switch ((bits + 7) / 8) {
        case 1:
            return ConstructGeneric1Byte(bits, impl);
        case 2:
            return ConstructGeneric2Bytes(bits, impl);
        case 3:
            return ConstructGeneric3Bytes(bits, impl);
        case 4:
            return ConstructGeneric4Bytes(bits, impl);
        case 5:
            return ConstructGeneric5Bytes(bits, impl);
        case 6:
            return ConstructGeneric6Bytes(bits, impl);
        case 7:
            return ConstructGeneric7Bytes(bits, impl);
        case 8:
            return ConstructGeneric8Bytes(bits, impl);
        default:
            return nullptr;
        }
        break;
#ifdef HAVE_CLMUL
    case FieldImpl::CLMUL:
        if (EnableClmul()) {
            switch ((bits + 7) / 8) {
            case 1:
                return ConstructClMul1Byte(bits, impl);
            case 2:
                return ConstructClMul2Bytes(bits, impl);
            case 3:
                return ConstructClMul3Bytes(bits, impl);
            case 4:
                return ConstructClMul4Bytes(bits, impl);
            case 5:
                return ConstructClMul5Bytes(bits, impl);
            case 6:
                return ConstructClMul6Bytes(bits, impl);
            case 7:
                return ConstructClMul7Bytes(bits, impl);
            case 8:
                return ConstructClMul8Bytes(bits, impl);
            default:
                return nullptr;
            }
        }
        break;
    case FieldImpl::CLMUL_TRI:
        if (EnableClmul()) {
            switch ((bits + 7) / 8) {
            case 1:
                return ConstructClMulTri1Byte(bits, impl);
            case 2:
                return ConstructClMulTri2Bytes(bits, impl);
            case 3:
                return ConstructClMulTri3Bytes(bits, impl);
            case 4:
                return ConstructClMulTri4Bytes(bits, impl);
            case 5:
                return ConstructClMulTri5Bytes(bits, impl);
            case 6:
                return ConstructClMulTri6Bytes(bits, impl);
            case 7:
                return ConstructClMulTri7Bytes(bits, impl);
            case 8:
                return ConstructClMulTri8Bytes(bits, impl);
            default:
                return nullptr;
            }
        }
        break;
#endif
    }
    return nullptr;
}

}

extern "C" {

int minisketch_bits_supported(uint32_t bits) {
#ifdef ENABLE_FIELD_INT_2
    if (bits == 2) return true;
#endif
#ifdef ENABLE_FIELD_INT_3
    if (bits == 3) return true;
#endif
#ifdef ENABLE_FIELD_INT_4
    if (bits == 4) return true;
#endif
#ifdef ENABLE_FIELD_INT_5
    if (bits == 5) return true;
#endif
#ifdef ENABLE_FIELD_INT_6
    if (bits == 6) return true;
#endif
#ifdef ENABLE_FIELD_INT_7
    if (bits == 7) return true;
#endif
#ifdef ENABLE_FIELD_INT_8
    if (bits == 8) return true;
#endif
#ifdef ENABLE_FIELD_INT_9
    if (bits == 9) return true;
#endif
#ifdef ENABLE_FIELD_INT_10
    if (bits == 10) return true;
#endif
#ifdef ENABLE_FIELD_INT_11
    if (bits == 11) return true;
#endif
#ifdef ENABLE_FIELD_INT_12
    if (bits == 12) return true;
#endif
#ifdef ENABLE_FIELD_INT_13
    if (bits == 13) return true;
#endif
#ifdef ENABLE_FIELD_INT_14
    if (bits == 14) return true;
#endif
#ifdef ENABLE_FIELD_INT_15
    if (bits == 15) return true;
#endif
#ifdef ENABLE_FIELD_INT_16
    if (bits == 16) return true;
#endif
#ifdef ENABLE_FIELD_INT_17
    if (bits == 17) return true;
#endif
#ifdef ENABLE_FIELD_INT_18
    if (bits == 18) return true;
#endif
#ifdef ENABLE_FIELD_INT_19
    if (bits == 19) return true;
#endif
#ifdef ENABLE_FIELD_INT_20
    if (bits == 20) return true;
#endif
#ifdef ENABLE_FIELD_INT_21
    if (bits == 21) return true;
#endif
#ifdef ENABLE_FIELD_INT_22
    if (bits == 22) return true;
#endif
#ifdef ENABLE_FIELD_INT_23
    if (bits == 23) return true;
#endif
#ifdef ENABLE_FIELD_INT_24
    if (bits == 24) return true;
#endif
#ifdef ENABLE_FIELD_INT_25
    if (bits == 25) return true;
#endif
#ifdef ENABLE_FIELD_INT_26
    if (bits == 26) return true;
#endif
#ifdef ENABLE_FIELD_INT_27
    if (bits == 27) return true;
#endif
#ifdef ENABLE_FIELD_INT_28
    if (bits == 28) return true;
#endif
#ifdef ENABLE_FIELD_INT_29
    if (bits == 29) return true;
#endif
#ifdef ENABLE_FIELD_INT_30
    if (bits == 30) return true;
#endif
#ifdef ENABLE_FIELD_INT_31
    if (bits == 31) return true;
#endif
#ifdef ENABLE_FIELD_INT_32
    if (bits == 32) return true;
#endif
#ifdef ENABLE_FIELD_INT_33
    if (bits == 33) return true;
#endif
#ifdef ENABLE_FIELD_INT_34
    if (bits == 34) return true;
#endif
#ifdef ENABLE_FIELD_INT_35
    if (bits == 35) return true;
#endif
#ifdef ENABLE_FIELD_INT_36
    if (bits == 36) return true;
#endif
#ifdef ENABLE_FIELD_INT_37
    if (bits == 37) return true;
#endif
#ifdef ENABLE_FIELD_INT_38
    if (bits == 38) return true;
#endif
#ifdef ENABLE_FIELD_INT_39
    if (bits == 39) return true;
#endif
#ifdef ENABLE_FIELD_INT_40
    if (bits == 40) return true;
#endif
#ifdef ENABLE_FIELD_INT_41
    if (bits == 41) return true;
#endif
#ifdef ENABLE_FIELD_INT_42
    if (bits == 42) return true;
#endif
#ifdef ENABLE_FIELD_INT_43
    if (bits == 43) return true;
#endif
#ifdef ENABLE_FIELD_INT_44
    if (bits == 44) return true;
#endif
#ifdef ENABLE_FIELD_INT_45
    if (bits == 45) return true;
#endif
#ifdef ENABLE_FIELD_INT_46
    if (bits == 46) return true;
#endif
#ifdef ENABLE_FIELD_INT_47
    if (bits == 47) return true;
#endif
#ifdef ENABLE_FIELD_INT_48
    if (bits == 48) return true;
#endif
#ifdef ENABLE_FIELD_INT_49
    if (bits == 49) return true;
#endif
#ifdef ENABLE_FIELD_INT_50
    if (bits == 50) return true;
#endif
#ifdef ENABLE_FIELD_INT_51
    if (bits == 51) return true;
#endif
#ifdef ENABLE_FIELD_INT_52
    if (bits == 52) return true;
#endif
#ifdef ENABLE_FIELD_INT_53
    if (bits == 53) return true;
#endif
#ifdef ENABLE_FIELD_INT_54
    if (bits == 54) return true;
#endif
#ifdef ENABLE_FIELD_INT_55
    if (bits == 55) return true;
#endif
#ifdef ENABLE_FIELD_INT_56
    if (bits == 56) return true;
#endif
#ifdef ENABLE_FIELD_INT_57
    if (bits == 57) return true;
#endif
#ifdef ENABLE_FIELD_INT_58
    if (bits == 58) return true;
#endif
#ifdef ENABLE_FIELD_INT_59
    if (bits == 59) return true;
#endif
#ifdef ENABLE_FIELD_INT_60
    if (bits == 60) return true;
#endif
#ifdef ENABLE_FIELD_INT_61
    if (bits == 61) return true;
#endif
#ifdef ENABLE_FIELD_INT_62
    if (bits == 62) return true;
#endif
#ifdef ENABLE_FIELD_INT_63
    if (bits == 63) return true;
#endif
#ifdef ENABLE_FIELD_INT_64
    if (bits == 64) return true;
#endif
    return false;
}

uint32_t minisketch_implementation_max() {
    uint32_t ret = 0;
#ifdef HAVE_CLMUL
    ret += 2;
#endif
    return ret;
}

int minisketch_implementation_supported(uint32_t bits, uint32_t implementation) {
    if (!minisketch_bits_supported(bits) || implementation > minisketch_implementation_max()) {
        return 0;
    }
    try {
        Sketch* sketch = Construct(bits, implementation);
        if (sketch) {
            delete sketch;
            return 1;
        }
    } catch (const std::bad_alloc&) {}
    return 0;
}

minisketch* minisketch_create(uint32_t bits, uint32_t implementation, size_t capacity) {
    try {
        Sketch* sketch = Construct(bits, implementation);
        if (sketch) {
            try {
                sketch->Init(capacity);
            } catch (const std::bad_alloc&) {
                delete sketch;
                throw;
            }
            sketch->Ready();
        }
        return (minisketch*)sketch;
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

uint32_t minisketch_bits(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    return s->Bits();
}

size_t minisketch_capacity(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    return s->Syndromes();
}

uint32_t minisketch_implementation(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    return s->Implementation();
}

minisketch* minisketch_clone(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    Sketch* r = (Sketch*) minisketch_create(s->Bits(), s->Implementation(), s->Syndromes());
    if (r) {
        r->Merge(s);
    }
    return (minisketch*) r;
}

void minisketch_destroy(minisketch* sketch) {
    if (sketch) {
        Sketch* s = (Sketch*)sketch;
        s->UnReady();
        delete s;
    }
}

size_t minisketch_serialized_size(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    size_t bits = s->Bits();
    size_t syndromes = s->Syndromes();
    return (bits * syndromes + 7) / 8;
}

void minisketch_serialize(const minisketch* sketch, unsigned char* output) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    s->Serialize(output);
}

void minisketch_deserialize(minisketch* sketch, const unsigned char* input) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->Deserialize(input);
}

void minisketch_add_uint64(minisketch* sketch, uint64_t element) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->Add(element);
}

size_t minisketch_merge(minisketch* sketch, const minisketch* other_sketch) {
    Sketch* s1 = (Sketch*)sketch;
    const Sketch* s2 = (const Sketch*)other_sketch;
    s1->Check();
    s2->Check();
    if (s1->Bits() != s2->Bits()) return 0;
    if (s1->Implementation() != s2->Implementation()) return 0;
    return s1->Merge(s2);
}

ssize_t minisketch_decode(const minisketch* sketch, size_t max_elements, uint64_t* output) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    return s->Decode(max_elements, output);
}

void minisketch_set_seed(minisketch* sketch, uint64_t seed) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->SetSeed(seed);
}

size_t minisketch_compute_capacity(uint32_t bits, size_t max_elements, uint32_t fpbits) {
    return ComputeCapacity(bits, max_elements, fpbits);
}

size_t minisketch_compute_max_elements(uint32_t bits, size_t capacity, uint32_t fpbits) {
    return ComputeMaxElements(bits, capacity, fpbits);
}

}
