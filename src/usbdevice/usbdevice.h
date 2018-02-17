// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_USBDEVICE_H
#define PARTICL_USBDEVICE_H

#include <string.h>
#include <assert.h>
#include <vector>
#include <string>

#include <key/extkey.h>
#include <script/sign.h>
#include <keystore.h>


enum DeviceTypeID {
    USBDEVICE_UNKNOWN = -1,
    USBDEVICE_LEDGER_NANO_S = 0,
};

class UniValue;
struct hid_device_;
typedef struct hid_device_ hid_device;

class CPathKey
{
public:
    std::vector<uint32_t> vPath;
    CPubKey pk;
};

class CPathKeyStore : public CBasicKeyStore
{
public:
    std::map<CKeyID, CPathKey> mapPathKeys;

    bool AddKey(const CPathKey &pathkey)
    {
        LOCK(cs_KeyStore);
        mapPathKeys[pathkey.pk.GetID()] = pathkey;
        return true;
    }

    bool GetKey(const CKeyID &address, CPathKey &keyOut) const
    {
        LOCK(cs_KeyStore);
        std::map<CKeyID, CPathKey>::const_iterator mi = mapPathKeys.find(address);
        if (mi != mapPathKeys.end()) {
            keyOut = mi->second;
            return true;
        }
        return false;
    }
    bool GetPubKey(const CKeyID &address, CPubKey &pkOut) const override
    {
        LOCK(cs_KeyStore);
        std::map<CKeyID, CPathKey>::const_iterator mi = mapPathKeys.find(address);
        if (mi != mapPathKeys.end()) {
            pkOut = mi->second.pk;
            return true;
        }
        return false;
    }
};

class DeviceType
{
public:
    DeviceType(
        int nVendorId_, int nProductId_,
        const char *cVendor_, const char *cProduct_,
        DeviceTypeID type_)
        : nVendorId(nVendorId_), nProductId(nProductId_),
          cVendor(cVendor_), cProduct(cProduct_), type(type_)
        {};

    int nVendorId = 0;
    int nProductId = 0;
    const char *cVendor = nullptr;
    const char *cProduct = nullptr;
    DeviceTypeID type = USBDEVICE_UNKNOWN;
};

class CUSBDevice
{
public:
    CUSBDevice(const DeviceType *pType_, const char *cPath_, const char *cSerialNo_, int nInterface_) : pType(pType_)
    {
        assert(strlen(cPath_) < sizeof(cPath));
        assert(strlen(cSerialNo_) < sizeof(cSerialNo));

        strcpy(cPath, cPath_);
        strcpy(cSerialNo, cSerialNo_);

        nInterface = nInterface_;
    };

    int Open();
    int Close();

    int GetFirmwareVersion(std::string &sFirmware, std::string &sError);
    int GetInfo(UniValue &info, std::string &sError);

    int GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError);
    int GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError);

    int SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError);

    //int SignHash(const std::vector<uint32_t> &vPath, const uint256 &hash, std::vector<uint8_t> &vchSig, std::string &sError);
    int SignTransaction(hid_device *handle, const std::vector<uint32_t> &vPath, const CTransaction *tx,
        int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t> &amount, SigVersion sigversion,
        std::vector<uint8_t> &vchSig, std::string &sError);
    int SignTransaction(const std::vector<uint32_t> &vPath, const CTransaction *tx,
        int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t> &amount, SigVersion sigversion,
        std::vector<uint8_t> &vchSig, std::string &sError);


    const DeviceType *pType = nullptr;
    char cPath[128];
    char cSerialNo[128];
    int nInterface;
    std::string sError;

private:
    hid_device *handle = nullptr;
};

void ListDevices(std::vector<CUSBDevice> &vDevices);


/** A signature creator for transactions. */
class DeviceSignatureCreator : public BaseSignatureCreator {
    const CTransaction* txTo;
    unsigned int nIn;
    int nHashType;
    std::vector<uint8_t> amount;
    const TransactionSignatureChecker checker;
    CUSBDevice *pDevice;


public:
    DeviceSignatureCreator(CUSBDevice *pDeviceIn, const CKeyStore *keystoreIn, const CTransaction *txToIn, unsigned int nInIn, const std::vector<uint8_t> &amountIn, int nHashTypeIn=SIGHASH_ALL);
    const BaseSignatureChecker &Checker() const { return checker; }

    bool IsParticlVersion() const { return txTo && txTo->IsParticlVersion(); }
    bool IsCoinStake() const { return txTo && txTo->IsCoinStake(); }

    bool CreateSig(std::vector<unsigned char> &vchSig, const CKeyID &keyid, const CScript &scriptCode, SigVersion sigversion) const override;
};



#endif // PARTICL_USBDEVICE_H

