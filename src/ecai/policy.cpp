#include <ecai/policy.h>
#include <cstdint>
#include <cassert>

namespace {

struct F {
    uint64_t feerate{0};
    uint16_t script_mask{0};
    uint16_t dust{0};
    uint16_t rbf{0};
    bool     have{false};
};

static inline uint64_t be64(const unsigned char* p){
    uint64_t v=0; for(int i=0;i<8;i++){v=(v<<8)|p[i];} return v;
}
static inline uint32_t be32(const unsigned char* p){
    uint32_t v=0; for(int i=0;i<4;i++){v=(v<<8)|p[i];} return v;
}
static inline uint16_t be16(const unsigned char* p){
    return (uint16_t)(p[0]<<8|p[1]);
}

static F ParseFeat(const std::vector<unsigned char>& tlv)
{
    F f;
    for (size_t i=0;i+1<tlv.size();) {
        uint8_t tag=tlv[i++]; uint8_t len=tlv[i++];
        const unsigned char* d=&tlv[i]; i+=len;
        switch(tag){
            case 1: f.feerate=be64(d); f.have=true; break;
            case 5: f.rbf=be16(d); break;
            case 6: f.script_mask=be16(d); break;
            case 7: f.dust=be16(d); break;
            default: break;
        }
    }
    return f;
}

} // anon

namespace ecai {

Verdict Evaluate(const std::vector<unsigned char>& pol,
                 const std::vector<unsigned char>& feat)
{
    F f = ParseFeat(feat);
    if (!f.have) return Verdict::REJECT;

    bool allow_ok = true;
    for (size_t i=0;i+1<pol.size();) {
        uint8_t op = pol[i++]; uint8_t len = pol[i++];
        const unsigned char* d=&pol[i]; i+=len;

        switch (op) {
            case 0x10: { // REQUIRE_FEERATE_GE u64
                uint64_t th = be64(d);
                if (f.feerate < th) return Verdict::REJECT;
            } break;
            case 0x11: { // REJECT_IF_RBF_AND_FEERATE_LT u64
                uint64_t th = be64(d);
                if (f.rbf && f.feerate < th) return Verdict::REJECT;
            } break;
            case 0x12: { // CAP_DUST_LE u16
                uint16_t th = be16(d);
                if (f.dust > th) return Verdict::REJECT;
            } break;
            case 0x13: { // ALLOW_SCRIPT_MASK u16
                uint16_t mask = be16(d);
                if (mask && ((mask & f.script_mask) == 0)) allow_ok = false;
            } break;
            case 0x20: { // QUEUE_IF_FEERATE_BETWEEN u64|u64
                uint64_t a = be64(d);
                uint64_t b = be64(d+8);
                if (f.feerate >= a && f.feerate < b) return Verdict::QUEUE;
            } break;
            default: break; // ignore unknown opcodes to keep forward-compat
        }
    }
    if (!allow_ok) return Verdict::REJECT;
    return Verdict::ACCEPT;
}

} // namespace ecai
