// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/ledgerdevice.h>

#include <hidapi/hidapi.h>
#include <stdio.h>
#include <inttypes.h>
#include <util.h>
#include <pubkey.h>
#include <crypto/common.h>
#include <utilstrencodings.h>
#include <univalue.h>
#include <chainparams.h>
#include <validation.h>

#ifdef ENABLE_WALLET
#include <wallet/hdwallet.h>
#endif

extern "C" {
#include <usbdevice/ledger/btchipApdu.h>
#include <usbdevice/ledger/dongleCommHidHidapi.h>
}

static const char *GetLedgerString(int code)
{
    switch (code)
    {
        case SW_OK:
            return "Ok";
        case SW_SECURITY_STATUS_NOT_SATISFIED:
            return "Security status not satisfied";
        case SW_CONDITIONS_OF_USE_NOT_SATISFIED:
            return "Conditions of use not satisfied";
        case SW_INCORRECT_DATA:
            return "Incorrect data";
        case SW_NOT_ENOUGH_MEMORY_SPACE:
            return "Not enough memory";
        case SW_REFERENCED_DATA_NOT_FOUND:
            return "Not enough memory";
        case SW_FILE_ALREADY_EXISTS:
            return "File already exists";
        case SW_INCORRECT_P1_P2:
            return "Incorrect p1 p2";
        case SW_INS_NOT_SUPPORTED:
            return "INS not supported";
        case SW_CLA_NOT_SUPPORTED:
            return "CLA not supported";
        case SW_TECHNICAL_PROBLEM:
            return "Technical problem";
        case SW_MEMORY_PROBLEM:
            return "Memory problem";
        case SW_NO_EF_SELECTED:
            return "No EF selected";
        case SW_INVALID_OFFSET:
            return "Invalid offset";
        case SW_FILE_NOT_FOUND:
            return "File not found";
        case SW_INCONSISTENT_FILE:
            return "Inconsistent file";
        case SW_ALGORITHM_NOT_SUPPORTED:
            return "Algorithm not supported";
        case SW_INVALID_KCV:
            return "Invalid KCV";
        case SW_CODE_NOT_INITIALIZED:
            return "Code not initialised";
        case SW_ACCESS_CONDITION_NOT_FULFILLED:
            return "Access condition not fulfilled";
        case SW_CONTRADICTION_SECRET_CODE_STATUS:
            return "Contradiction secret code status";
        case SW_CONTRADICTION_INVALIDATION:
            return "Contradiction invalidation";
        case SW_CODE_BLOCKED:
            return "Code blocked";
        case SW_MAX_VALUE_REACHED:
            return "Max value reached";
        case SW_GP_AUTH_FAILED:
            return "GP auth failed";
        case SW_LICENSING:
            return "Licencing";
        case SW_HALTED:
            return "Halted";
        default:
            break;
    };
    return "Unknown";
};

int CLedgerDevice::Open()
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

int CLedgerDevice::Close()
{
    if (handle)
        hid_close(handle);
    handle = nullptr;

    hid_exit();
    return 0;
};

int CLedgerDevice::GetFirmwareVersion(std::string &sFirmware, std::string &sError)
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

int CLedgerDevice::GetInfo(UniValue &info, std::string &sError)
{
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_GET_COIN_VER;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;

    int sw;
    int result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);

    if (sw != SW_OK)
    {
        Close();
        return errorN(1, sError, __func__, "Dongle application error: %.4x", sw);
    };
    if (result < 3)
    {
        Close();
        return errorN(1, sError, __func__, "Bad read length: %d", result);
    };


    UniValue errors(UniValue::VARR);

    std::vector<uint8_t> data(1);
    data[0] = out[1];
    info.pushKV("pubkey_version", strprintf("%.2x", data[0]));
    if (data != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS))
        errors.push_back("pubkey version mismatch.");

    data[0] = out[3];
    info.pushKV("script_version", strprintf("%.2x", data[0]));
    info.pushKV("coin_family", strprintf("%.2x", out[4]));

    if (out[5] <= 128)
    {
        std::string s((const char*)&out[6], out[5]);
        info.pushKV("coin_id", s);
    };

    apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_GET_OPERATION_MODE;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;

    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    if (sw == SW_OK)
    {
        int om = out[0];
        info.pushKV("operation_mode", strprintf("%.2x %s", om,
            om == BTCHIP_MODE_ISSUER            ? "issuer" :
            om == BTCHIP_MODE_SETUP_NEEDED      ? "setup needed" :
            om == BTCHIP_MODE_WALLET            ? "wallet" :
            om == BTCHIP_MODE_RELAXED_WALLET    ? "relaxed wallet" :
            om == BTCHIP_MODE_SERVER            ? "server" :
            om == BTCHIP_MODE_DEVELOPER         ? "developer" :
            "unknown"));
    } else
    {
        errors.push_back("Get operation mode failed.");
    };

    if (errors.size() > 0)
        info.pushKV("errors", errors);

    Close();

    return 0;
};

int CLedgerDevice::GetPubKey(const std::vector<uint32_t> &vPath, CPubKey &pk, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH)
        return errorN(1, sError, __func__, _("Path depth out of range.").c_str());
    size_t lenPath = vPath.size();
    if (0 != Open())
        return errorN(1, sError, __func__, "Failed to open device.");

    uint8_t in[260];
    uint8_t out[260];
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
    Close();

    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle application error: %.4x %s", sw, GetLedgerString(sw));
    if (result < 65)
        return errorN(1, sError, __func__, "Bad read length: %d", result);

    size_t ofs = 0;
    size_t lenPubkey = out[ofs++];
    if (lenPubkey != 33 && lenPubkey != 65)
        return errorN(1, sError, __func__, "Bad pubkey size: %d", lenPubkey);

    pk.Set(&out[ofs], &out[ofs+lenPubkey]);
    if (lenPubkey == 65 && !pk.Compress())
        return errorN(1, sError, __func__, "Pubkey compression failed.");

    return 0;
};

int CLedgerDevice::GetXPub(const std::vector<uint32_t> &vPath, CExtPubKey &ekp, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH)
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

int CLedgerDevice::SignMessage(const std::vector<uint32_t> &vPath, const std::string &sMessage, std::vector<uint8_t> &vchSig, std::string &sError)
{
    if (vPath.size() < 1 || vPath.size() > MAX_BIP32_PATH)
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

int CLedgerDevice::PrepareTransaction(const CMutableTransaction *tx, const CCoinsViewCache &view)
{
    if (!handle)
        return errorN(1, sError, __func__, "Device not open.");
    int result, sw;
    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;

    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x02; // segwit
    in[apduSize++] = 4 + GetSizeOfVarInt<VarIntMode::DEFAULT>(tx->vin.size());

    in[apduSize++] = tx->nVersion;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    apduSize += PutVarInt(&in[apduSize], tx->vin.size());

    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

    for (size_t i = 0; i < tx->vin.size(); ++i)
    {
        const auto &txin = tx->vin[i];
        const Coin &coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent())
            return errorN(1, sError, __func__, _("Input %d not found or already spent").c_str(), i);

        const CScript &scriptCode = coin.out.scriptPubKey;
        std::vector<uint8_t> vchAmount(8);
        memcpy(vchAmount.data(), &coin.out.nValue, 8);


        apduSize = 0;
        in[apduSize++] = BTCHIP_CLA;
        in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
        in[apduSize++] = 0x80;
        in[apduSize++] = 0x00;
        size_t ofslen = apduSize++;

        in[apduSize++] = 0x02; // segwit

        memcpy(&in[apduSize], txin.prevout.hash.begin(), 32);
        apduSize += 32;
        memcpy(&in[apduSize], &txin.prevout.n, 4);
        apduSize += 4;

        if (vchAmount.size() != 8)
            return errorN(1, sError, __func__, "amount must be 8 bytes.");
        memcpy(&in[apduSize], vchAmount.data(), vchAmount.size());
        apduSize += vchAmount.size();

        if (i == 0)
        {
            apduSize += PutVarInt(&in[apduSize], scriptCode.size());
        } else
        {
            apduSize += PutVarInt(&in[apduSize], 0); // empty scriptCode
            memcpy(&in[apduSize], &txin.nSequence, 4);
            apduSize += 4;
        };

        in[ofslen] = apduSize - (ofslen+1);

        result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
        if (sw != SW_OK)
            return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

        size_t offset = 0;
        const size_t blockLength = 255;
        if (i == 0)
        while(offset < scriptCode.size())
        {
            size_t dataLength = (offset + blockLength) < scriptCode.size()
                ? blockLength : scriptCode.size() - offset;

            apduSize = 0;
            in[apduSize++] = BTCHIP_CLA;
            in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
            in[apduSize++] = 0x80;
            in[apduSize++] = 0x00;
            size_t ofslen = apduSize++;
            memcpy(&in[apduSize], &scriptCode[offset], dataLength);
            apduSize += dataLength;

            if ((offset + dataLength) == scriptCode.size())
            {
                memcpy(&in[apduSize], &txin.nSequence, 4);
                apduSize += 4;
            };

            in[ofslen] = apduSize - (ofslen+1);

            result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
            if (sw != SW_OK)
                return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

            offset += dataLength;
        };
    };

    // finalizeInputFull

    std::vector<uint8_t> vOutputData;
    PutVarInt(vOutputData, tx->vpout.size());
    for (const auto &txout : tx->vpout)
    {
        std::vector<uint8_t> vchAmount(8);
        if (txout->IsType(OUTPUT_DATA))
        {
            memset(vchAmount.data(), 0, vchAmount.size());
            vOutputData.insert(vOutputData.end(), vchAmount.begin(), vchAmount.end());

            std::vector<uint8_t> &vData = ((CTxOutData*)txout.get())->vData;
            PutVarInt(vOutputData, vData.size());
            vOutputData.insert(vOutputData.end(), vData.begin(), vData.end());
            continue;
        };

        if (!txout->IsStandardOutput())
            return errorN(1, sError, __func__, "all outputs must be standard.");
        CAmount nValue = txout->GetValue();
        memcpy(&vchAmount[0], &nValue, 8);
        vOutputData.insert(vOutputData.end(), vchAmount.begin(), vchAmount.end());

        const CScript *pScript = txout->GetPScriptPubKey();
        PutVarInt(vOutputData, pScript->size());
        vOutputData.insert(vOutputData.end(), pScript->begin(), pScript->end());
    };

    //std::vector<uint8_t> vEncryptedOutput;
    size_t offset = 0;
    const size_t scriptBlockLength = 50;
    while (offset < vOutputData.size())
    {
        size_t dataLength;
        uint8_t p1;
        if ((offset + scriptBlockLength) < vOutputData.size())
        {
            dataLength = scriptBlockLength;
            p1 = 0x00;
        } else
        {
            dataLength = vOutputData.size() - offset;
            p1 = 0x80;
        };

        apduSize = 0;
        in[apduSize++] = BTCHIP_CLA;
        in[apduSize++] = BTCHIP_INS_HASH_INPUT_FINALIZE_FULL;
        in[apduSize++] = p1;
        in[apduSize++] = 0x00;
        in[apduSize++] = dataLength;
        memcpy(&in[apduSize], &vOutputData[offset], dataLength);
        apduSize += dataLength;

        result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
        if (sw != SW_OK || result < 0)
            return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

        offset += dataLength;
    };

    return 0;
};

int CLedgerDevice::SignTransaction(const std::vector<uint32_t> &vPath, const std::vector<uint8_t> &vSharedSecret, const CMutableTransaction *tx,
    int nIn, const CScript &scriptCode, int hashType, const std::vector<uint8_t>& amount, SigVersion sigversion,
    std::vector<uint8_t> &vchSig, std::string &sError)
{
    if (!handle)
        return errorN(1, sError, __func__, "Device not open.");
    int result, sw;
    uint8_t in[260];
    uint8_t out[260];
    size_t apduSize = 0;

    const auto &txin = tx->vin[nIn];

    // startUntrustedTransaction
    apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x80; // !newTransaction
    in[apduSize++] = 4 + 1; // GetSizeOfVarInt(1);

    in[apduSize++] = tx->nVersion;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    apduSize += PutVarInt(&in[apduSize], 1);

    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

    apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
    in[apduSize++] = 0x80;
    in[apduSize++] = 0x00;
    size_t ofslen = apduSize++;

    in[apduSize++] = 0x02; // segwit
    if (amount.size() != 8)
        return errorN(1, sError, __func__, "amount must be 8 bytes.");

    memcpy(&in[apduSize], txin.prevout.hash.begin(), 32);
    apduSize += 32;
    memcpy(&in[apduSize], &txin.prevout.n, 4);
    apduSize += 4;
    memcpy(&in[apduSize], amount.data(), amount.size());
    apduSize += amount.size();

    apduSize += PutVarInt(&in[apduSize], scriptCode.size());

    in[ofslen] = apduSize - (ofslen+1);

    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));

    size_t offset = 0;
    const size_t blockLength = 255;
    while(offset < scriptCode.size())
    {
        size_t dataLength = (offset + blockLength) < scriptCode.size()
            ? blockLength : scriptCode.size() - offset;

        apduSize = 0;
        in[apduSize++] = BTCHIP_CLA;
        in[apduSize++] = BTCHIP_INS_HASH_INPUT_START;
        in[apduSize++] = 0x80;
        in[apduSize++] = 0x00;
        size_t ofslen = apduSize++;
        memcpy(&in[apduSize], &scriptCode[offset], dataLength);
        apduSize += dataLength;

        if ((offset + dataLength) == scriptCode.size())
        {
            memcpy(&in[apduSize], &txin.nSequence, 4);
            apduSize += 4;
        };

        in[ofslen] = apduSize - (ofslen+1);

        result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
        if (sw != SW_OK)
            return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));
        offset += dataLength;
    };

    // untrustedHashSign

    apduSize = 0;
    in[apduSize++] = BTCHIP_CLA;
    in[apduSize++] = BTCHIP_INS_HASH_SIGN;
    in[apduSize++] = 0x00;
    in[apduSize++] = 0x00;
    ofslen = apduSize++;
    in[apduSize++] = vPath.size();
    for (size_t k = 0; k < vPath.size(); k++, apduSize+=4)
        WriteBE32(&in[apduSize], vPath[k]);

    in[apduSize++] = 0x00; // len(pin)

    WriteBE32(&in[apduSize], tx->nLockTime);
    apduSize += 4;
    in[apduSize++] = hashType;
    in[apduSize++] = vSharedSecret.size();
    if (vSharedSecret.size() > 0)
    {
        memcpy(&in[apduSize], vSharedSecret.data(), vSharedSecret.size());
        apduSize += vSharedSecret.size();
    };

    in[ofslen] = apduSize - (ofslen+1);
    result = sendApduHidHidapi(handle, 1, in, apduSize, out, sizeof(out), &sw);
    if (sw != SW_OK)
        return errorN(1, sError, __func__, "Dongle error: %.4x %s", sw, GetLedgerString(sw));
    if (result < 70)
        return errorN(1, sError, __func__, "Bad read length: %d", result);

    out[0] = 0x30; // ?
    vchSig.resize(result);
    memcpy(vchSig.data(), out, result);

    return 0;
};
