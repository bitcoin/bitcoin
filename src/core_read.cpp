
#include <vector>
#include "core_io.h"
#include "core.h"
#include "serialize.h"

using namespace std;

bool DecodeHexTx(CTransaction& tx, const std::string& strHexTx)
{
    if (!IsHex(strHexTx))
        return false;

    vector<unsigned char> txData(ParseHex(strHexTx));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        return false;
    }

    return true;
}

