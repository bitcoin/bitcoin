#include <script/address.h>
#include <key_io.h>

DestinationAddr::DestinationAddr(const CTxDestination& dest)
{
    if (dest.type() == typeid(StealthAddress)) {
        m_script = boost::get<StealthAddress>(dest);
    } else {
        m_script = ::GetScriptForDestination(dest);
    }
}

std::string DestinationAddr::Encode() const
{
    CTxDestination dest;
    if (ExtractDestination(dest)) {
        return ::EncodeDestination(dest);
    }

    return HexStr(GetScript());
}

const CScript& DestinationAddr::GetScript() const noexcept
{
    assert(m_script.type() == typeid(CScript));
    return boost::get<CScript>(m_script);
}

const StealthAddress& DestinationAddr::GetMWEBAddress() const noexcept
{
    assert(m_script.type() == typeid(StealthAddress));
    return boost::get<StealthAddress>(m_script);
}

bool DestinationAddr::ExtractDestination(CTxDestination& dest) const
{
    if (IsMWEB()) {
        dest = GetMWEBAddress();
        return true;
    }
    return ::ExtractDestination(GetScript(), dest);
}