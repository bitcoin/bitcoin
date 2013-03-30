#ifndef _SECP256K1_FIELD_
#define _SECP256K1_FIELD_

// just one implementation for now
#include "field_5x52.h"

namespace secp256k1 {

class FieldConstants {
public:
    secp256k1_num_t field_p;

    FieldConstants();
    ~FieldConstants();
};

const FieldConstants &GetFieldConst();

}

#endif
