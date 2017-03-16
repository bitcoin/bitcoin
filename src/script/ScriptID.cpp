#include "ScriptID.h"
#include "BitcoinAddress.h"

base58string CScriptID::GetBase58addressWithNetworkScriptPrefix() const
{
    CBase58Data data;
    data.SetData(Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS), this, 20); // ###### script の prefix
    return data.ToBase58string();
}
