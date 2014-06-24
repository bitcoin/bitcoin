#ifndef __BITCOIN_CORE_IO_H__
#define __BITCOIN_CORE_IO_H__

#include <string>

class CScript;
class CTransaction;

// core_read.cpp
extern CScript ParseScript(std::string s);
extern bool DecodeHexTx(CTransaction& tx, const std::string& strHexTx);

// core_write.cpp
extern std::string EncodeHexTx(const CTransaction& tx);

#endif // __BITCOIN_CORE_IO_H__
