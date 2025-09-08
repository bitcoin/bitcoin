#include <ecai/h2c.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <hash.h>
#include <string.h>

namespace ecai {

static secp256k1_context* Ctx()
{
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    return ctx;
}

// Tagged hash per BIP340 style, tag = "ECAI_H2C"
static uint256 TaggedHashH2C(const unsigned char* data, size_t len, uint32_t ctr)
{
    static const std::string tag = "ECAI_H2C";
    uint256 taghash = Hash(tag.begin(), tag.end());
    CHashWriter ss(SER_GETHASH, 0);
    ss << taghash << taghash;
    ss.write((const char*)data, len);
    ss << ctr;
    return ss.GetHash();
}

bool HashToCurve(const std::vector<unsigned char>& msg,
                 std::array<unsigned char,33>& out)
{
    const auto* ctx = Ctx();
    secp256k1_pubkey pk;
    // Try-and-increment: interpret 32B as X, try y parity 0 then 1
    // Build compressed: 0x02/0x03 || X
    for (uint32_t i=0;i<1'000'000;i++) {
        uint256 h = TaggedHashH2C(msg.data(), msg.size(), i);
        unsigned char x32[32]; memcpy(x32, h.begin(), 32);
        for (int parity=0; parity<2; ++parity) {
            unsigned char comp[33]; comp[0] = parity? 0x03 : 0x02;
            memcpy(comp+1, x32, 32);
            if (secp256k1_ec_pubkey_parse(ctx, &pk, comp, 33)) {
                memcpy(out.data(), comp, 33);
                return true;
            }
        }
    }
    return false; // extremely unlikely
}

} // namespace ecai
