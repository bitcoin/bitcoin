#include <blsct/common.h>

std::vector<uint8_t> blsct::Common::CDataSteamToVector(const CDataStream& st)
{
    auto data = reinterpret_cast<const int8_t*>(st.data());
    std::vector<uint8_t> vec;
    std::copy(data, data + st.size(), std::back_inserter(vec));
    return vec;
}

size_t blsct::Common::GetFirstPowerOf2GreaterOrEqTo(const size_t& n)
{
    size_t i = 1;
    while (i < n) {
        i *= 2;
    }
    return i;
}