#include <primitives/dynnft.h>


void CNFTAssetClass::writeString(std::string data)
{

    uint32_t len = data.size();
    strSerialData.push_back((len & 0xFF000000) >> 24);
    strSerialData.push_back((len & 0x00FF0000) >> 16);
    strSerialData.push_back((len & 0x0000FF00) >> 8);
    strSerialData.push_back((len & 0x000000FF));
    for (int i = 0; i < data.size(); i++)
        strSerialData.push_back(data[i]);

}

int readString(std::vector<unsigned char> vec, std::string data, int start) {

    uint32_t len = (uint32_t)vec[start] << 24;
    len += (uint32_t)vec[start + 1] << 16;
    len += (uint32_t)vec[start + 2] << 8;
    len += (uint32_t)vec[start + 3];

    data.clear();
    for (int i = 0; i < len; i++)
        data.push_back(vec[start + i + 4]);

    return start + len + 4;
}

void CNFTAssetClass::writeVector(std::vector<unsigned char> data)
{
    uint32_t len = data.size();
    strSerialData.push_back((len & 0xFF000000) >> 24);
    strSerialData.push_back((len & 0x00FF0000) >> 16);
    strSerialData.push_back((len & 0x0000FF00) >> 8);
    strSerialData.push_back((len & 0x000000FF));
    for (int i = 0; i < data.size(); i++)
        strSerialData.push_back(data[i]);
}

int readVector(std::vector<unsigned char> vec, std::vector<unsigned char>  data, int start)
{
    uint32_t len = (uint32_t)vec[start] << 24;
    len += (uint32_t)vec[start + 1] << 16;
    len += (uint32_t)vec[start + 2] << 8;
    len += (uint32_t)vec[start + 3];

    data.clear();
    for (int i = 0; i < len; i++)
        data.push_back(vec[start + i + 4]);

    return start + len + 4;
}


CNFTAssetClass::CNFTAssetClass() {

    serialDataCreated = false;

}

void CNFTAssetClass::createSerialData() {
    if (serialDataCreated)
        return;

    strSerialData.clear();
    writeString( hash );
    writeString( metaData );
    writeString( owner );
    writeString( txnID );
    strSerialData.push_back(maxCount >> 56);
    strSerialData.push_back((maxCount & 0x00FF000000000000) >> 48);
    strSerialData.push_back((maxCount & 0x0000FF0000000000) >> 40);
    strSerialData.push_back((maxCount & 0x000000FF00000000) >> 32);
    strSerialData.push_back((maxCount & 0x00000000FF000000) >> 24);
    strSerialData.push_back((maxCount & 0x0000000000FF0000) >> 16);
    strSerialData.push_back((maxCount & 0x000000000000FF00) >> 8);
    strSerialData.push_back((maxCount & 0x00000000000000FF));


    serialDataCreated = true;
}


void CNFTAssetClass::loadFromSerialData(std::vector<unsigned char> data) {

    int ptr = 0;
    ptr = readString(data, hash, ptr);
    ptr = readString(data, metaData, ptr);
    ptr = readString(data, owner, ptr);
    ptr = readString(data, txnID, ptr);

    maxCount = (uint64_t)data[ptr] << 56;
    maxCount += (uint64_t)data[ptr + 1] << 48;
    maxCount += (uint64_t)data[ptr + 2] << 40;
    maxCount += (uint64_t)data[ptr + 3] << 32;
    maxCount += (uint64_t)data[ptr + 4] << 24;
    maxCount += (uint64_t)data[ptr + 5] << 16;
    maxCount += (uint64_t)data[ptr + 6] << 8;
    maxCount += (uint64_t)data[ptr + 7];
}


CNFTAsset::CNFTAsset() {

}
