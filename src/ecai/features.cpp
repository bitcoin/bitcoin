#include <ecai/features.h>
#include <ecai/h2c.h>
#include <script/standard.h>
#include <policy/fees.h>
#include <streams.h>

namespace ecai {

enum : uint8_t {
    F_FEERATE_SAT_VB = 1,
    F_VSIZE          = 2,
    F_NIN            = 3,
    F_NOUT           = 4,
    F_RBF            = 5,
    F_SCRIPT_MASK    = 6,
    F_DUST_COUNT     = 7,
};

static void PutTLV(std::vector<unsigned char>& out, uint8_t tag,
                   const unsigned char* data, size_t len)
{
    out.push_back(tag);
    out.push_back((unsigned char)len);
    out.insert(out.end(), data, data+len);
}

static void PutU16(std::vector<unsigned char>& out, uint8_t tag, uint16_t v)
{
    unsigned char b[2] {(unsigned char)(v>>8), (unsigned char)v};
    PutTLV(out, tag, b, 2);
}
static void PutU32(std::vector<unsigned char>& out, uint8_t tag, uint32_t v)
{
    unsigned char b[4] {(unsigned char)(v>>24),(unsigned char)(v>>16),
                        (unsigned char)(v>>8), (unsigned char)v};
    PutTLV(out, tag, b, 4);
}
static void PutU64(std::vector<unsigned char>& out, uint8_t tag, uint64_t v)
{
    unsigned char b[8];
    for (int i=7;i>=0;--i){ b[7-i]=(unsigned char)(v>>(i*8)); }
    PutTLV(out, tag, b, 8);
}

static uint16_t ScriptTypeMask(const CTransaction& tx)
{
    // Bit 0: P2PKH, 1: P2WPKH, 2: P2WSH, 3: P2TR (very coarse)
    uint16_t m=0;
    for (const auto& o : tx.vout) {
        txnouttype which;
        std::vector<std::vector<unsigned char>> vSolutions;
        if (Solver(o.scriptPubKey, which, vSolutions)) {
            if (which==TX_PUBKEYHASH) m|=1<<0;
            if (which==TX_WITNESS_V0_KEYHASH) m|=1<<1;
            if (which==TX_WITNESS_V0_SCRIPTHASH) m|=1<<2;
            if (which==TX_WITNESS_V1_TAPROOT) m|=1<<3;
        }
    }
    return m;
}

std::vector<unsigned char> EncodeTLVFeatures(const CTransaction& tx,
                                             const CTxMemPool& /*mempool*/)
{
    std::vector<unsigned char> tlv; tlv.reserve(80);

    // Effective feerate estimate (sats/vB) from raw tx (no ancestors)
    const size_t vsize = GetVirtualTransactionSize(tx);
    CAmount fee = 0;
    for (const auto& in : tx.vin) fee += 0; // unknown here; keep 0 if not connected
    const uint64_t sat_vb = vsize ? (uint64_t)(fee * 1000 / vsize) : 0;

    const bool rbf = std::any_of(tx.vin.begin(), tx.vin.end(),
                                 [](const CTxIn& in){ return in.IsSequenceFinal() ? false : true; });

    uint16_t dust = 0;
    for (const auto& o : tx.vout) { if (o.nValue < 546) dust++; }

    PutU64(tlv, F_FEERATE_SAT_VB, sat_vb);
    PutU32(tlv, F_VSIZE, (uint32_t)vsize);
    PutU16(tlv, F_NIN,  (uint16_t)tx.vin.size());
    PutU16(tlv, F_NOUT, (uint16_t)tx.vout.size());
    PutU16(tlv, F_SCRIPT_MASK, ScriptTypeMask(tx));
    PutU16(tlv, F_DUST_COUNT, dust);
    PutU16(tlv, F_RBF, (uint16_t)(rbf?1:0));

    return tlv;
}

bool FeatureCommitment(const CTransaction& tx, const CTxMemPool& mempool,
                       std::array<unsigned char,33>& out)
{
    auto tlv = EncodeTLVFeatures(tx, mempool);
    return HashToCurve(tlv, out);
}

} // namespace ecai
