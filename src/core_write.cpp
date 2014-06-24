
#include "core_io.h"
#include "core.h"
#include "serialize.h"
#include "util.h"

using namespace std;

string EncodeHexTx(const CTransaction& tx)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    return HexStr(ssTx.begin(), ssTx.end());
}

