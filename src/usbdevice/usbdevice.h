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


enum DeviceTypeID {
    USBDEVICE_UNKNOWN = -1,
    USBDEVICE_LEDGER_NANO_S = 0,
};

class UniValue;
struct hid_device_;
typedef struct hid_device_ hid_device;

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

    //int GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError);
    int GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError);

    int SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError);

    const DeviceType *pType = nullptr;
    char cPath[128];
    char cSerialNo[128];
    int nInterface;

private:
    hid_device *handle = nullptr;
};

void ListDevices(std::vector<CUSBDevice> &vDevices);



#endif // PARTICL_USBDEVICE_H

