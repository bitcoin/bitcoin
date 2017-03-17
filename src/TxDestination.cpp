#include "TxDestination.h"
#include "BitcoinAddress.h"

base58string CTxDestination::GetBase58addressWithNetworkPrefix() const
{
    return CBitcoinAddress(*this).ToBase58string66();
}
