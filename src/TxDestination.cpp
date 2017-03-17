#include "TxDestination.h"
#include "BitcoinAddress.h"

namespace
{
    class CTxDestinationVisitor : public boost::static_visitor<base58string>
    {
    public:
        base58string operator()(const CKeyID& id) const { return id.GetBase58addressWithNetworkPubkeyPrefix(); }
        base58string operator()(const CScriptID& id) const { return id.GetBase58addressWithNetworkScriptPrefix(); }
        base58string operator()(const CNoDestination& no) const { return base58string(); } // ############## これはこれで合ってる？？要確認.
    };

} // anon namespace

base58string CTxDestination::GetBase58addressWithNetworkPrefix() const
{
    return boost::apply_visitor(CTxDestinationVisitor(), *this);
}

// ##### 要動作確認.
bool CTxDestination::IsNoDestination() const
{
    return this->type() == typeid(CNoDestination);
}
