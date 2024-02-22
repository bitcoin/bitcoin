#include <blsct/common.h>

std::vector<uint8_t> blsct::Common::DataStreamToVector(const DataStream& st)
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

template <typename T>
std::vector<T> blsct::Common::TrimPreceedingZeros(
    const std::vector<T>& vec
) {
    std::vector<T> trimmed_vec;

    bool should_take = false;
    for (auto x : vec) {
        if (!should_take && x) should_take = true;
        if (should_take) trimmed_vec.push_back(x);
    }
    return trimmed_vec;
}
template
std::vector<bool> blsct::Common::TrimPreceedingZeros(const std::vector<bool>&);
template
std::vector<uint8_t> blsct::Common::TrimPreceedingZeros(const std::vector<uint8_t>&);

template <typename T>
void blsct::Common::AddZeroIfEmpty(std::vector<T>& vec)
{
    if (vec.size() == 0) {
        vec.push_back(0);
    }
}
template
void blsct::Common::AddZeroIfEmpty(std::vector<bool>& vec);
template
void blsct::Common::AddZeroIfEmpty(std::vector<uint8_t>& vec);
