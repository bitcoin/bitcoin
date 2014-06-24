#ifndef __BITCOIN_CORE_IO_H__
#define __BITCOIN_CORE_IO_H__

#include <string>

class CTransaction;

// core_read.cpp
extern bool DecodeHexTx(CTransaction& tx, const std::string& strHexTx);

// core_write.cpp
extern std::string EncodeHexTx(const CTransaction& tx);

#endif // __BITCOIN_CORE_IO_H__
