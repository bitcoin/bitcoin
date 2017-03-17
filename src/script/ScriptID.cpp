#include "ScriptID.h"
#include "BitcoinAddress.h"

base58string CScriptID::GetBase58addressWithNetworkScriptPrefix() const
{
    base58string str;
    str.SetData(Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS), this, 20); // ###### script の prefix
    return str;
}
