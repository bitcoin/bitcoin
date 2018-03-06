// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/debugdevice.h>

#include <base58.h>
#include <univalue.h>
#include <validation.h>
#include <hash.h>

const char *seed = "debug key";

const size_t MAX_BIP32_PATH = 10;

CDebugDevice::CDebugDevice()
{
    pType = &usbDeviceTypes[USBDEVICE_DEBUG];
    strcpy(cPath, "none");
    strcpy(cSerialNo, "1");
    nInterface = 0;

    ekv.SetMaster((const uint8_t*)seed, strlen(seed));
};


int CDebugDevice::GetFirmwareVersion(std::string &sFirmware, std::string &sError)
{
    sFirmware = "debug v1";
    return 0;
};

int CDebugDevice::GetInfo(UniValue &info, std::string &sError)
{
    CBitcoinExtKey ekOut;
    ekOut.SetKey(ekv);
    info.pushKV("device", "debug");
    info.pushKV("extkey", ekOut.ToString());
    return 0;
};

int CDebugDevice::GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH)
        return errorN(1, sError, __func__, _("Path depth out of range.").c_str());

    CExtKey vkOut, vkWork = ekv;
    for (auto it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (!vkWork.Derive(vkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        vkWork = vkOut;
    };

    pk = vkOut.key.GetPubKey();

    return 0;
};

int CDebugDevice::GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH)
        return errorN(1, sError, __func__, _("Path depth out of range.").c_str());

    CExtKey vkOut, vkWork = ekv;
    for (auto it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (!vkWork.Derive(vkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        vkWork = vkOut;
    };

    ekp = vkOut.Neutered();

    return 0;
};

int CDebugDevice::SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError)
{
    CExtKey vkOut, vkWork = ekv;
    for (auto it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (!vkWork.Derive(vkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        vkWork = vkOut;
    };

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << sMessage;

    if (!vkOut.key.SignCompact(ss.GetHash(), vchSig))
        return errorN(1, sError, __func__, "Sign failed");

    return 0;
};

int CDebugDevice::PrepareTransaction(const CTransaction *tx, const CCoinsViewCache &view)
{
    return 0;
};

int CDebugDevice::SignTransaction(const std::vector<uint32_t> &vPath, const std::vector<uint8_t> &vSharedSecret, const CTransaction *tx,
    int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t> &amount, SigVersion sigversion,
    std::vector<uint8_t> &vchSig, std::string &sError)
{
    uint256 hash = SignatureHash(scriptCode, *tx, nIn, hashType, amount, sigversion);

    CExtKey vkOut, vkWork = ekv;
    for (auto it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (!vkWork.Derive(vkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        vkWork = vkOut;
    };

    CKey key = vkOut.key;
    if (vSharedSecret.size() == 32)
    {
        key = key.Add(vSharedSecret.data());
        if (!key.IsValid())
            return errorN(1, sError, __func__, "Add failed");
    };

    if (!key.Sign(hash, vchSig))
        return errorN(1, sError, __func__, "Sign failed");
    vchSig.push_back((unsigned char)hashType);

    return 0;
};
