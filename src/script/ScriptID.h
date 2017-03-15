#pragma once

#include "base58.h"

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public uint160 {
public:
    CScriptID() : uint160() {}
    CScriptID(const CScript& in);
    CScriptID(const uint160& in) : uint160(in) {}
public:
    base58string GetBase58addressWithNetworkScriptPrefix() const;
};
