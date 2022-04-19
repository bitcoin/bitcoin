#include <cowbytes.h>

CowBytes::CowBytes(Span<const std::byte> data) : m_data(data)
{
}

CowBytes::CowBytes(std::string data) : m_data(std::move(data))
{
}

Span<const std::byte> CowBytes::GetSpan() const
{
    if (m_data.valueless_by_exception()) {
        return Span<const std::byte>();
    }
    return std::visit([](const auto& d) { return MakeByteSpan(d); }, m_data);
}
