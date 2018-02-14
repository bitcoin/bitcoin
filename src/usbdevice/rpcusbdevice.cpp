// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/usbdevice.h>
#include <rpc/server.h>
#include <utilstrencodings.h>
#include <base58.h>
#include <key/extkey.h>
#include <chainparams.h>

#include <univalue.h>

static std::string GetDefaultPath()
{
    // return path of default account: 44'/44'/0'
    std::vector<uint32_t> vPath;
    vPath.push_back(WithHardenedBit(44)); // purpose
    vPath.push_back(WithHardenedBit(Params().BIP44ID())); // coin
    vPath.push_back(WithHardenedBit(0)); // account
    std::string rv;
    if (0 == PathToString(vPath, rv, 'h'))
        return rv;
    return "";
}

static std::vector<uint32_t> GetPath(std::vector<uint32_t> &vPath, const UniValue &path, const UniValue &defaultpath)
{
    // Pass empty string as defaultpath to drop defaultpath and use path as full path
    std::string sPath;
    if (path.isStr())
    {
        sPath = path.get_str();
    } else
    if (path.isNum())
    {
        sPath = strprintf("%d", path.get_int());
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown \"path\" type."));
    };

    if (defaultpath.isNull())
    {
        sPath = GetDefaultPath() + "/" + sPath;
    } else
    if (path.isNum())
    {
        sPath = strprintf("%d", defaultpath.get_int()) + "/" + sPath;
    } else
    if (defaultpath.isStr())
    {
        if (!defaultpath.get_str().empty())
            sPath = defaultpath.get_str() + "/" + sPath;
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown \"defaultpath\" type."));
    };

    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Bad path: %s.", ExtKeyGetString(rv)));

    return vPath;
}

UniValue listdevices(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 0)
        throw std::runtime_error(
            "listdevices\n"
            "list connected hardware devices.\n"
            "\nResult\n"
            "{\n"
            "  \"vendor\"           (string) USB vendor string.\n"
            "  \"product\"          (string) USB product string.\n"
            "  \"firmwareversion\"  (string) Detected firmware version of device, if possible.\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("listdevices", "")
            + HelpExampleRpc("listdevices", ""));

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    UniValue result(UniValue::VARR);

    for (auto &device : vDevices)
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
    };

    return result;
};


UniValue getdeviceinfo(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 0)
        throw std::runtime_error(
            "getdeviceinfo\n"
            "Get information from connected hardware device.\n"
            "\nResult\n"
            "{\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getdeviceinfo", "")
            + HelpExampleRpc("getdeviceinfo", ""));

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

UniValue getdevicepublickey(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "getdevicepublickey \"path\" (\"accountpath\")\n"
            "Get the public key and address at \"path\" from a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"              (string, required) The path to derive the key from.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"accountpath\"       (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "{\n"
            "  \"publickey\"        (string) The derived public key at \"path\".\n"
            "  \"address\"          (string) The address of \"publickey\".\n"
            "  \"path\"             (string) The full path of \"publickey\".\n"
            "}\n"
            "\nExamples\n"
            "Get the first public key of external chain:\n"
            + HelpExampleCli("getdevicepublickey", "\"0/0\"")
            + HelpExampleRpc("getdevicepublickey", "\"0/0\"")
            + "Get the first public key of internal chain of testnet account:\n"
            + HelpExampleCli("getdevicepublickey", "\"1/0\" \"44h/1h/0h\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[1]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    std::string sError;
    CPubKey pk;
    if (0 != vDevices[0].GetPubKey(vPath, pk, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetPubKey failed %s.", sError));

    std::string sPath;
    if (0 != PathToString(vPath, sPath))
        sPath = "error";
    UniValue rv(UniValue::VOBJ);
    rv.pushKV("publickey", HexStr(pk));
    rv.pushKV("address", CBitcoinAddress(pk.GetID()).ToString());
    rv.pushKV("path", sPath);
    return rv;
}

UniValue getdevicexpub(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "getdevicexpub \"path\" (\"accountpath\")\n"
            "Get the extended public key at \"path\" from a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"              (string, required) The path to derive the key from.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"accountpath\"       (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "\"address\"              (string) The particl extended public key\n"
            "\nExamples\n"
            + HelpExampleCli("getdevicexpub", "\"0\"")
            + HelpExampleRpc("getdevicexpub", "\"0\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[1]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    std::string sError;
    CExtPubKey ekp;
    if (0 != vDevices[0].GetXPub(vPath, ekp, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetXPub failed %s.", sError));

    return CBitcoinExtPubKey(ekp).ToString();
};

UniValue devicesignmessage(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw std::runtime_error(
            "devicesignmessage \"path\" \"message\" (\"accountpath\")\n"
            "Sign a message with the key at \"path\" on a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"            (string, required) The path to the key to sign with.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"message\"         (string, required) The message to create a signature for.\n"
            "3. \"accountpath\"     (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples\n"
            "Sign with the first key of external chain:\n"
            + HelpExampleCli("devicesignmessage", "\"0/0\" \"my message\"")
            + HelpExampleRpc("devicesignmessage", "\"0/0\", \"my message\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[2]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    if (!request.params[1].isStr())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Bad message.");

    std::string sError, sMessage = request.params[1].get_str();
    std::vector<uint8_t> vchSig;
    if (0 != vDevices[0].SignMessage(vPath, sMessage, vchSig, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("SignMessage failed %s.", sError));

    return EncodeBase64(vchSig.data(), vchSig.size());
};


static const CRPCCommand commands[] =
{ //  category              name                        actor (function)           argNames
  //  --------------------- ------------------------    -----------------------    ----------
    { "usbdevice",          "listdevices",              &listdevices,              {} },
    { "usbdevice",          "getdeviceinfo",            &getdeviceinfo,            {} },
    { "usbdevice",          "getdevicepublickey",       &getdevicepublickey,       {"path","accountpath"} },
    { "usbdevice",          "getdevicexpub",            &getdevicexpub,            {"path","accountpath"} },
    { "usbdevice",          "devicesignmessage",        &devicesignmessage,        {"path","message","accountpath"} },
};

void RegisterUSBDeviceRPC(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
