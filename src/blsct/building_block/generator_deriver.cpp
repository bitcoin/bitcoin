#include <blsct/building_block/generator_deriver.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/common.h>
#include <stdexcept>
#include <util/strencodings.h>
#include <hash.h>
#include <optional>
#include <variant>

template <typename Point>
Point GeneratorDeriver<Point>::Derive(
    const Point& p,
    const size_t index,
    const std::optional<Seed>& opt_seed
) const {
    std::vector<uint8_t> serialized_p = p.GetVch();

    auto num_to_str = [](auto n) {
        std::ostringstream os;
        os << n;
        return os.str();
    };
    std::string hash_preimage;

    if (std::holds_alternative<TokenId>(opt_seed.value())) {
        auto token_id = std::get<TokenId>(opt_seed.value());
        hash_preimage =
            HexStr(serialized_p) +
            salt_str +
            num_to_str(index) +
            token_id.ToString() +
            "nft" + num_to_str(token_id.subid)
        ;
    } else if (std::holds_alternative<blsct::Message>(opt_seed.value())) {
        auto message = std::get<Bytes>(opt_seed.value());
        hash_preimage =
            HexStr(serialized_p) +
            salt_str +
            num_to_str(index) +
            "DERIVATION_FROM_MESSAGE" +
            HexStr(message)
        ;
    } else {
        throw new std::runtime_error("Unexpected seed type");
    }
    HashWriter ss{};
    ss << hash_preimage;
    auto hash = ss.GetHash();

    auto vec_hash = std::vector<uint8_t>(hash.begin(), hash.end());
    auto ret = Point::MapToPoint(vec_hash);
    if (ret.IsZero()) {
        throw std::runtime_error(strprintf(
            "%s: Generated G1Point is the point at infinity. Try changing parameters", __func__));
    }
    return ret;
}
template
typename Mcl::Point GeneratorDeriver<typename Mcl::Point>::Derive(
    const Mcl::Point& p,
    const size_t index,
    const std::optional<Seed>& seed
) const;
