// just one implementation for now
#include "field_5x52.cpp"

namespace secp256k1 {

static const unsigned char field_p_[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F};

FieldConstants::FieldConstants() {
    secp256k1_num_init(&field_p);
    secp256k1_num_set_bin(&field_p, field_p_, sizeof(field_p_));
}

FieldConstants::~FieldConstants() {
    secp256k1_num_free(&field_p);
}

const FieldConstants &GetFieldConst() {
    static const FieldConstants field_const;
    return field_const;
}

}
