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
#include <memory>

enum DeviceTypeID {
    USBDEVICE_UNKNOWN = -1,
    USBDEVICE_DEBUG = 0,
    USBDEVICE_LEDGER_NANO_S = 1,
    USBDEVICE_SIZE,
};

class UniValue;
struct hid_device_;
typedef struct hid_device_ hid_device;
class CCoinsViewCache;

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

extern const DeviceType usbDeviceTypes[];

class CUSBDevice
{
public:
    CUSBDevice() {};
    CUSBDevice(const DeviceType *pType_, const char *cPath_, const char *cSerialNo_, int nInterface_) : pType(pType_)
    {
        assert(strlen(cPath_) < sizeof(cPath));
        assert(strlen(cSerialNo_) < sizeof(cSerialNo));

        strcpy(cPath, cPath_);
        strcpy(cSerialNo, cSerialNo_);

        nInterface = nInterface_;
    };

    virtual int Open() { return 0; };
    virtual int Close() { return 0; };

    virtual int GetFirmwareVersion(std::string &sFirmware, std::string &sError);
    virtual int GetInfo(UniValue &info, std::string &sError);

    virtual int GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError) { return 0; };
    virtual int GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError) { return 0; };

    virtual int SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError) { return 0; };

    virtual int PrepareTransaction(const CTransaction *tx, const CCoinsViewCache &view) { return 0; };

    //int SignHash(const std::vector<uint32_t> &vPath, const uint256 &hash, std::vector<uint8_t> &vchSig, std::string &sError);
    virtual int SignTransaction(const std::vector<uint32_t> &vPath, const std::vector<uint8_t> &vSharedSecret, const CTransaction *tx,
        int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t> &amount, SigVersion sigversion,
        std::vector<uint8_t> &vchSig, std::string &sError) { return 0; };


    const DeviceType *pType = nullptr;
    char cPath[128];
    char cSerialNo[128];
    int nInterface;
    std::string sError;

protected:
    hid_device *handle = nullptr;
};

void ListDevices(std::vector<std::unique_ptr<CUSBDevice> > &vDevices);
CUSBDevice *SelectDevice(std::vector<std::unique_ptr<CUSBDevice> > &vDevices, std::string &sError);

/*
class QueuedSignature
{
public:

};
*/
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

