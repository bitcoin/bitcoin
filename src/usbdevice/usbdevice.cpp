// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/usbdevice.h>


#include <hidapi/hidapi.h>
#include <stdio.h>
#include <inttypes.h>
#include <util.h>
#include <pubkey.h>
#include <crypto/common.h>
#include <utilstrencodings.h>
#include <univalue.h>
#include <chainparams.h>

extern "C" {
#include <usbdevice/ledger/btchipApdu.h>
#include <usbdevice/ledger/dongleCommHidHidapi.h>
}

const char *GetLedgerString(int code)
{
    switch (code)
    {
        case SW_OK:
            return "Ok";
        case SW_SECURITY_STATUS_NOT_SATISFIED:
            return "Security status not satisfied";
        default:
            break;
    };
    return "Unknown";
};

static const DeviceType usbDeviceTypes[] = {
    DeviceType(0x2c97, 0x0001, "Ledger", "Nano S", USBDEVICE_LEDGER_NANO_S),
};

int CUSBDevice::Open()
{
    if (!pType)
        return 1;

    if (hid_init())
        return 1;

    if (!(handle = hid_open_path(cPath)))
    {
        hid_exit();
        return 1;
    };

    return 0;
};

int CUSBDevice::Close()
{
    if (handle)
        hid_close(handle);
    handle = nullptr;

    hid_exit();
    return 0;
};


int CUSBDevice::GetFirmwareVersion(std::string &sFirmware, std::string &sError)
{
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_GET_FIRMWARE_VERSION;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;

    int sw;
    int result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    Close();

    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle application error: %.4x", sw);
    if (result < 5)
        return errorN(1, sError, __func__, "Bad read length: %d", result);

    sFirmware = strprintf("%s %d.%d.%d", (out[1] != 0 ? "Ledger" : ""), out[2], out[3], out[4]);

    return 0;
};

int CUSBDevice::GetInfo(UniValue &info, std::string &sError)
{
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = 0x16; // getCoinVersion
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;

    int sw;
    int result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    Close();

    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle application error: %.4x", sw);
    if (result < 2)
        return errorN(1, sError, __func__, "Bad read length: %d", result);


    UniValue errors(UniValue::VARR);

    std::vector<uint8_t> vchVersion;
    vchVersion.push_back(out[1]);
    info.pushKV("pubkeyversion", strprintf("%.2x", vchVersion[0]));

    if (vchVersion != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
        errors.push_back("pubkeyversion mismatch.");

    if (errors.size() > 0)
        info.pushKV("errors", errors);

    return 0;
};

int CUSBDevice::GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH) // 10, in firmware
        return errorN(1, sError, __func__, _("Path depth out of range.").c_str());
    size_t lenPath = vPath.size();
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
    uint8_t outB[260];
    size_t apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_GET_WALLET_PUBLIC_KEY;
    in[apduSize++] = 0x00;      // show on device
    in[apduSize++] = 0x00;      // segwit
    in[apduSize++] = 1 + 4 * lenPath; // num bytes to follow
    in[apduSize++] = lenPath;
    for (size_t k = 0; k < vPath.size(); k++, apduSize+=4)
        WriteBE32(&in[apduSize], vPath[k]);

    int sw;
    int result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);

    // Get fingerprint
    if (sw == SW_OK && lenPath > 1 && result > 65)
    {
        size_t lenPathParent = lenPath-1;
        in[4] = 1 + 4 * lenPathParent; // num bytes to follow
        in[5] = lenPathParent;
        apduSize-=4;

        result = sendApduHidHidapi(handle, 1, in, apduSize, outB, sizeof(outB), &sw);
    };
    Close();

    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle application error: %.4x %s", sw, GetLedgerString(sw));
    if (result < 65)
        return errorN(1, sError, __func__, "Bad read length: %d", result);

    ekp.nDepth = lenPath;
    ekp.nChild = vPath.back();
    memset(ekp.vchFingerprint, 0, 4);

    size_t ofs = 0;
    size_t lenPubkey = out[ofs++];
    if (lenPubkey != 33 && lenPubkey != 65)
        return errorN(1, sError, __func__, "Bad pubkey size: %d", lenPubkey);

    ekp.pubkey.Set(&out[ofs], &out[ofs+lenPubkey]);
    if (lenPubkey == 65 && !ekp.pubkey.Compress())
        return errorN(1, sError, __func__, "Pubkey compression failed.");
    ofs+=lenPubkey;
    size_t lenAddr = out[ofs++];
    ofs+=lenAddr;
    memcpy(ekp.vchChainCode, &out[ofs], 32);

    // Set fingerprint
    if (lenPath > 1)
    {
        size_t ofs = 0;
        size_t lenPubkey = outB[ofs++];
        if (lenPubkey != 33 && lenPubkey != 65)
            return errorN(1, sError, __func__, "Bad pubkey size: %d", lenPubkey);
        CPubKey pkParent(&outB[ofs], &outB[ofs+lenPubkey]);

        if (lenPubkey == 65 && !pkParent.Compress())
            return errorN(1, sError, __func__, "Pubkey compression failed.");
        CKeyID id = pkParent.GetID();
        memcpy(&ekp.vchFingerprint[0], &id, 4);
    };

    return 0;
};

int CUSBDevice::SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH) // 10, in firmware
        return errorN(1, sError, __func__, _("Path depth out of range.").c_str());
    size_t lenPath = vPath.size();
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_SIGN_MESSAGE;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = lenPath;
    for (size_t k = 0; k < vPath.size(); k++, apduSize+=4)
        WriteBE32(&in[apduSize], vPath[k]);
    size_t slen = sMessage.size();
    if (slen > sizeof(in) - apduSize)
        return errorN(1, sError, __func__, _("Message too long.").c_str());

    in[apduSize++] = slen;
    memcpy(in + apduSize, sMessage.c_str(), slen);
    apduSize += slen;
    in[OFFSET_CDATA] = (apduSize - 5);

    int sw;
    int result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);

    if (sw != SW_OK)
    {
        Close();
        return errorN(1, sError, __func__, "Dongle application error: %.4x %s", sw, GetLedgerString(sw));
    };
    if (result < 1)
    {
        Close();
        return errorN(1, sError, __func__, "Bad read length: %d", result);
    };
    if (out[0] != 0x00)
    {
        Close();
        return errorN(1, sError, __func__, "Message signature prepared, please powercycle to get the second factor then proceed with signing");
    };


    apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_SIGN_MESSAGE;
    in[apduSize++] = 0x80;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[OFFSET_CDATA] = (apduSize - 5);
    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);

    Close();

    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle application error: %.4x %s", sw, GetLedgerString(sw));
    if (result < 70)
        return errorN(1, sError, __func__, "Bad read length: %d", result);



    size_t lenR = out[3];
    size_t lenS = out[4 + lenR + 1];
    if (lenR < 32 || lenR > 33)
        return errorN(1, sError, __func__, "Bad r length: %d", lenR);
    if (lenS < 32 || lenS > 33)
        return errorN(1, sError, __func__, "Bad s length: %d", lenS);

    vchSig.resize(65);
    vchSig[0] = 27 + 4 + (out[0] & 0x01);

    if (lenR == 32)
        memcpy(&vchSig[1], &out[4], 32);
    else
        memcpy(&vchSig[1], &out[5], 32);
    if (lenS == 32)
        memcpy(&vchSig[33], &out[4+lenR+2], 32);
    else
        memcpy(&vchSig[33], &out[4+lenR+3], 32);

    return 0;
};


void ListDevices(std::vector<CUSBDevice> &vDevices)
{
    struct hid_device_info *devs, *cur_dev;

    if (hid_init())
        return;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    while (cur_dev) {

        for (const auto &type : usbDeviceTypes)
        {
            if (cur_dev->vendor_id != type.nVendorId
                || cur_dev->product_id != type.nProductId)
                continue;
            vDevices.emplace_back(&type, cur_dev->path, (char*)cur_dev->serial_number, cur_dev->interface_number);
        };
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    hid_exit();
    return;
};

