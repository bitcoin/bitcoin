#include "TxDestination.h"
#include "BitcoinAddress.h"

// #### このへんは仮.
base58string CTxDestination::GetBase58addressWithNetworkPrefix() const
{
    return CBitcoinAddress(*this).m_data.ToBase58string();
}

// ##### 要動作確認.
bool CTxDestination::IsNoDestination() const
{
    return this->type() == typeid(CNoDestination);
}
