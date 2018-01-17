// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "systemnodeconfig.h"
#include "systemnode.h"
#include "systemnodeman.h"
#include "activesystemnode.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include "wallet.h"
#include "key.h"
#include "base58.h"

#include "updater.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include <boost/thread.hpp>

using namespace json_spirit;
using namespace std;

namespace RPCUpdate
{
    Object statusObj;
    bool Started = false;
    void progressFunction(curl_off_t now, curl_off_t total)
    {
        int percent = 0;
        if (total != 0) {
            percent = now * 100 / total;
        }
        if (statusObj.size() == 0) {
            statusObj.push_back(Pair("Download", "In Progress"));
        }
        if ((now == total) && now != 0) {
            Started = false;
            statusObj[0] = Pair("Download", "Done");
        } else if (now != total) {
            Started = true;
            statusObj[0] = Pair("Download", strprintf("%0.1f/%0.1fMB, %d%%",
                                            double(now) / 1024 / 1024, 
                                            double(total) / 1024 / 1024,
                                            percent));
        }
    }
}

void RPCDownload()
{
    RPCUpdate::statusObj.clear();
    // Create temporary directory
    boost::filesystem::path dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    bool result = TryCreateDirectory(dir);
    if (!result) {
        throw runtime_error("Failed to create directory" + dir.string());
    }

    // Download archive
    std::string url = updater.GetDownloadUrl();
    std::string archivePath = (dir / boost::filesystem::path(url).filename()).string();
    updater.DownloadFile(url, archivePath, &RPCUpdate::progressFunction);
    RPCUpdate::statusObj[0] = Pair("Download", "Done - " + archivePath);
}

void RPCInstall()
{
    RPCUpdate::statusObj.clear();
    // Create temporary directory
    boost::filesystem::path dir = GetTempPath() / boost::filesystem::unique_path();
    bool result = TryCreateDirectory(dir);
    if (!result) {
        throw runtime_error("Failed to create directory" + dir.string());
    }

    // Download archive
    std::string url = updater.GetDownloadUrl();
    std::string archivePath = (dir / boost::filesystem::path(url).filename()).string();
    updater.DownloadFile(url, archivePath, &RPCUpdate::progressFunction);

    // Extract archive
    result = TryCreateDirectory(dir / "archive");
    if (!result) {
        throw runtime_error("Failed to create directory" + (dir / "archive").string());
    }
    std::string strCommand = "unzip -q " + archivePath + " -d " + (dir / "archive").string();
    int nErr = ::system(strCommand.c_str());
    if (nErr) {
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
        RPCUpdate::statusObj.push_back(Pair("Extract", "Error. Check debug.log"));
    } else {
        RPCUpdate::statusObj.push_back(Pair("Extract", "Done"));
    }

    // Copy files to /usr/
    if (!nErr) {
        strCommand = "cp -r " + (dir / "archive/*/*").string() + " /usr/";
        nErr = ::system(strCommand.c_str());
        if (nErr) {
            LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
            RPCUpdate::statusObj.push_back(Pair("Install", "Error. Check debug.log."));
        } else {
            RPCUpdate::statusObj.push_back(Pair("Install", "Done"));
        }
    }

    boost::filesystem::remove_all(dir);
}

Value update(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  || (strCommand != "check" && strCommand != "download" && strCommand != "status" && strCommand != "install"))
        throw runtime_error(
                "update \"command\"... ( \"passphrase\" )\n"
                "Set of commands to check and download application updates\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  check        - Check for application updates\n"
                "  download     - Download a new version\n"
                "  status       - Check download status\n"
                "  install      - Install update\n"
                );

    if (strCommand == "check")
    {
        if (params.size() > 1) {
            throw runtime_error("Too many parameters\n");
        }

        if (updater.Check())
        {
            // There is an update
            Object obj;
            obj.push_back(Pair("Current Version", FormatVersion(CLIENT_VERSION)));
            obj.push_back(Pair("Update Version", FormatVersion(updater.GetVersion())));
            obj.push_back(Pair("OS", updater.GetOsString()));
            obj.push_back(Pair("Url", updater.GetDownloadUrl()));
            obj.push_back(Pair("Sha256hash", updater.GetDownloadSha256Sum()));
            return obj;
        }
        return "You are running the latest version of Crown - " + FormatVersion(CLIENT_VERSION);
    }
    
    if (strCommand == "download")
    {
        if (!updater.Check())
        {
            return "You are running the latest version of Crown - " + FormatVersion(CLIENT_VERSION);
        }
        if (RPCUpdate::Started)
        {
            return "Download is in progress. Run 'crown-cli update status' to check the status.";
        }
        boost::thread t(boost::bind(&RPCDownload));
        return "Crown download started. \nRun 'crown-cli update status' to check download status.";
    }

    if (strCommand == "status")
    {
        return RPCUpdate::statusObj;
    }

    if (strCommand == "install")
    {
        if (!updater.Check())
        {
            return "You are running the latest version of Crown - " + FormatVersion(CLIENT_VERSION);
        }
        if (RPCUpdate::Started)
        {
            return "Download is in progress. Run 'crown-cli update status' to check the status.";
        }
        boost::thread t(boost::bind(&RPCInstall));
        return "Crown install started. \nRun 'crown-cli update status' to check download status.";
    }
    return "";
}
