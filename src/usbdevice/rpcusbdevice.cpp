// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "usbdevice/usbdevice.h"
#include "rpc/server.h"
#include "utilstrencodings.h"
#include "base58.h"
#include "key/extkey.h"

#include <univalue.h>

UniValue listdevices(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "listdevices\n"
            "list of available hardware devices.\n");

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    UniValue result(UniValue::VARR);

    for (const auto &device : vDevices)
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("vendor", device.pType->cVendor);
        obj.pushKV("product", device.pType->cProduct);

        std::string sValue, sError;
        if (0 == device.GetFirmwareVersion(sValue, sError))
            obj.pushKV("firmwareversion", sValue);
        else
            obj.pushKV("error", sError);

        result.push_back(obj);
    }

    return result;
};


UniValue getdeviceinfo(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getdeviceinfo\n"
            "Get info from hardware device.\n");

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    UniValue info(UniValue::VOBJ);
    std::string sError;
    if (0 != vDevices[0].GetInfo(info, sError))
        info.pushKV("error", sError);

    return info;
};

UniValue getdevicexpub(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getdevicexpub path\n"
            "Get extended public key from hardware device.\n");

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");

    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    std::string sError;
    CExtPubKey ekp;
    std::vector<uint32_t> vPath;

    if (request.params[0].isStr())
    {
        int rv;
        if ((rv = ExtractExtKeyPath(request.params[0].get_str(), vPath)) != 0)
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Bad path: %s.", ExtKeyGetString(rv)));
    } else
    {
        vPath.push_back(0);
    };

    if (0 != vDevices[0].GetXPub(vPath, ekp, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetXPub failed %s.", sError));

    return CBitcoinExtPubKey(ekp).ToString();
    //UniValue result(UniValue::VARR);
    //result.pushKV("xpub", CBitcoinExtPubKey(ekp).ToString());
    //return result;
};

UniValue devicesignmessage(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "devicesignmessage path message\n"
            "Sign message.\n");

    UniValue result(UniValue::VARR);
    return result;
};


static const CRPCCommand commands[] =
{ //  category              name                        actor (function)           okSafeMode
  //  --------------------- ------------------------    -----------------------    ----------
    { "usbdevice",          "listdevices",              &listdevices,              false,  {} },
    { "usbdevice",          "getdeviceinfo",            &getdeviceinfo,            false,  {} },
    { "usbdevice",          "getdevicexpub",            &getdevicexpub,            false,  {} },
    { "usbdevice",          "devicesignmessage",        &devicesignmessage,        false,  {} },
};

void RegisterUSBDeviceRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
