// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_LEDGERDEVICE_H
#define PARTICL_LEDGERDEVICE_H

#include <usbdevice/usbdevice.h>

class CLedgerDevice : public CUSBDevice
{
public:
    CLedgerDevice(const DeviceType *pType_, const char *cPath_, const char *cSerialNo_, int nInterface_)
        : CUSBDevice(pType_, cPath_, cSerialNo_, nInterface_) {};

    int Open() override;
    int Close() override;

    int GetFirmwareVersion(std::string &sFirmware, std::string &sError) override;
    int GetInfo(UniValue &info, std::string &sError) override;

    int GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError) override;
    int GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError) override;

    int SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError) override;

    int PrepareTransaction(const CTransaction *tx, const CCoinsViewCache &view) override;

    int SignTransaction(const std::vector<uint32_t> &vPath, const std::vector<uint8_t> &vSharedSecret, const CTransaction *tx,
        int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t> &amount, SigVersion sigversion,
        std::vector<uint8_t> &vchSig, std::string &sError) override;
};

#endif // PARTICL_LEDGERDEVICE_H
