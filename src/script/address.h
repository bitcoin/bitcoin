#pragma once

#include <script/standard.h>
#include <boost/variant.hpp>

//
// A wrapper around a boost::variant<CScript, StealthAddress> to deal with cases where
// a raw CScript is passed around as an address, rather than a CTxDestination.
// Since StealthAddresses can't be represented as a CScript, this wrapper should be used
// wherever the address could contain a StealthAddress instead of a CScript
//
class DestinationAddr
{
public:
    DestinationAddr() = default;
    DestinationAddr(CScript script)
        : m_script(std::move(script)) {}
    DestinationAddr(StealthAddress address)
        : m_script(std::move(address)) {}
    DestinationAddr(const CTxDestination& dest);

    bool operator==(const DestinationAddr& rhs) const noexcept { return this->m_script == rhs.m_script; }
    bool operator<(const DestinationAddr& rhs) const noexcept { return this->m_script < rhs.m_script; }
    bool operator<=(const DestinationAddr& rhs) const noexcept { return this->m_script <= rhs.m_script; }

    std::string Encode() const;

    bool IsMWEB() const noexcept { return m_script.type() == typeid(StealthAddress); }
    bool IsEmpty() const noexcept { return !IsMWEB() && GetScript().empty(); }

    const CScript& GetScript() const noexcept;
    const StealthAddress& GetMWEBAddress() const noexcept;
    bool ExtractDestination(CTxDestination& dest) const;

private:
    boost::variant<CScript, StealthAddress> m_script;
};