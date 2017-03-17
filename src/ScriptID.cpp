#include "ScriptID.h"
#include "BitcoinAddress.h"

base58string CScriptID::GetBase58addressWithNetworkScriptPrefix() const
{
    return CBitcoinAddress(*this).ToBase58string66();
}
