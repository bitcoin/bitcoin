#ifndef BITCOIN_COWBYTES_H
#define BITCOIN_COWBYTES_H

#include <variant>
#include <vector>
#include <string>

#include <span.h>

/**
 * @brief The CowBytes class; a class that represents either bytes owned by its object or a thin wrapper (span) around an already owned bytes with no owner
 */
class CowBytes
{
    std::variant<std::string, Span<const std::byte>> m_data;

public:
    explicit CowBytes(Span<const std::byte> data);
    explicit CowBytes(std::string data);

    Span<const std::byte> GetSpan() const;
};

#endif // BITCOIN_COWBYTES_H
