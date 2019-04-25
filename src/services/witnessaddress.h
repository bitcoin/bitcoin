#ifndef WITNESSADDRESS_H
#define WITNESSADDRESS_H
#include <serialize.h>
class CWitnessAddress{
public:
    unsigned char nVersion;
    std::vector<unsigned char> vchWitnessProgram;
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nVersion);
        READWRITE(vchWitnessProgram);
    }
    CWitnessAddress(const unsigned char &version, const std::vector<unsigned char> &vchWitnessProgram_) {
        nVersion = version;
        vchWitnessProgram = vchWitnessProgram_;
    }
    CWitnessAddress() {
        SetNull();
    }

    CWitnessAddress(CWitnessAddress && other){
        nVersion = std::move(other.nVersion);
        vchWitnessProgram = std::move(other.vchWitnessProgram);
    }
    
    
    CWitnessAddress& operator=(const CWitnessAddress& other) {
        this->nVersion = other.nVersion;
        this->vchWitnessProgram = other.vchWitnessProgram;
        return *this;
    }
    CWitnessAddress& operator=(CWitnessAddress&& other){
    
       if (this != &other)
       {
          nVersion = std::move(other.nVersion);
          vchWitnessProgram = std::move(other.vchWitnessProgram);
       }
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
    bool IsValid() const;
    std::string ToString() const;
    inline bool IsNull() const {
        return (nVersion == 0 && vchWitnessProgram.empty());
    }
};  
#endif // WITNESSADDRESS_H