// Copyright (c) 2017-2020 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_WITNESSADDRESS_H
#define SYSCOIN_SERVICES_WITNESSADDRESS_H
#include <serialize.h>
class CWitnessAddress{
public:
    unsigned char nVersion;
    std::vector<unsigned char> vchWitnessProgram;
    SERIALIZE_METHODS(CWitnessAddress, obj)
    {
        READWRITE(obj.nVersion, obj.vchWitnessProgram);
    }
    CWitnessAddress(const unsigned char &version, const std::vector<unsigned char> &vchWitnessProgram_) {
        nVersion = version;
        vchWitnessProgram = vchWitnessProgram_;
    }
    CWitnessAddress() {
        SetNull();
    } 
    
    CWitnessAddress& operator=(const CWitnessAddress& other) {
        this->nVersion = other.nVersion;
        this->vchWitnessProgram = other.vchWitnessProgram;
        return *this;
    }
 
    inline bool operator==(const CWitnessAddress& other) const {
        return this->nVersion == other.nVersion && this->vchWitnessProgram == other.vchWitnessProgram;
    }
    inline bool operator!=(const CWitnessAddress& other) const {
        return (this->nVersion != other.nVersion || this->vchWitnessProgram != other.vchWitnessProgram);
    }
    inline bool operator< (const CWitnessAddress& right) const
    {
        return ToString() < right.ToString();
    }
    inline void SetNull() {
        nVersion = 0;
        vchWitnessProgram.clear();
    }
    std::string ToString() const;
    bool IsValid() const;
    inline bool IsNull() const {
        return (nVersion == 0 && vchWitnessProgram.empty());
    }
};
#endif // SYSCOIN_SERVICES_WITNESSADDRESS_H