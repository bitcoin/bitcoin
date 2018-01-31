// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcupdate.h"

#include "init.h"
#include "clientversion.h"
#include "rpcserver.h"
#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

using namespace json_spirit;
using namespace std;

bool RPCUpdate::started = false;
Object RPCUpdate::statusObj;

void RPCUpdate::Download()
{
    statusObj.clear();
    // Create temporary directory
    boost::filesystem::path dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    bool result = TryCreateDirectory(dir);
    if (!result) {
        throw runtime_error("Failed to create directory" + dir.string());
    }

    // Download archive
    std::string url = updater.GetDownloadUrl();
    std::string archivePath = (dir / boost::filesystem::path(url).filename()).string();
    updater.DownloadFile(url, archivePath, &ProgressFunction);
    if (CheckSha(archivePath))
    {
        statusObj[0] = Pair("Download", "Done - " + archivePath);
    }
    else
    {
        statusObj[0] = Pair("Download", "Error. SHA-256 verification failed.");
        boost::filesystem::remove_all(dir);
    }
}

void RPCUpdate::Install()
{
    statusObj.clear();
    // Create temporary directory
    boost::filesystem::path dir = GetTempPath() / boost::filesystem::unique_path();
    bool result = TryCreateDirectory(dir);
    if (!result) {
        throw runtime_error("Failed to create directory" + dir.string());
    }

    // Download archive
    std::string url = updater.GetDownloadUrl();
    std::string archivePath = (dir / boost::filesystem::path(url).filename()).string();
    updater.DownloadFile(url, archivePath, &ProgressFunction);
    if (CheckSha(archivePath))
    {
        statusObj[0] = Pair("Download", "Done");
    }
    else
    {
        statusObj[0] = Pair("Download", "Error. SHA-256 verification failed.");
        boost::filesystem::remove_all(dir);
        return;
    }

    // Extract archive
    result = TryCreateDirectory(dir / "archive");
    if (!result) {
        throw runtime_error("Failed to create directory" + (dir / "archive").string());
    }
    std::string strCommand = "unzip -q " + archivePath + " -d " + (dir / "archive").string();
    int nErr = ::system(strCommand.c_str());
    if (nErr) {
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
        statusObj.push_back(Pair("Extract", "Error. Check debug.log"));
    } else {
        statusObj.push_back(Pair("Extract", "Done"));
    }

    // Copy files to /usr/
    if (!nErr) {
        strCommand = "cp -r " + (dir / "archive/*/*").string() + " /usr/";
        nErr = ::system(strCommand.c_str());
        if (nErr) {
            LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
            statusObj.push_back(Pair("Install", "Error. Check debug.log."));
        } else {
            statusObj.push_back(Pair("Install", "Done"));
        }

        // Restart crownd
        StartRestart();
    }

    boost::filesystem::remove_all(dir);
}

void RPCUpdate::ProgressFunction(curl_off_t now, curl_off_t total)
{
    int percent = 0;
    if (total != 0) {
        percent = now * 100 / total;
    }
    if (statusObj.size() == 0) {
        statusObj.push_back(Pair("Download", "In Progress"));
    }
    if ((now == total) && now != 0) {
        started = false;
        statusObj[0] = Pair("Download", "Done");
    } else if (now != total) {
        started = true;
        statusObj[0] = Pair("Download", strprintf("%0.1f/%0.1fMB, %d%%",
                                        static_cast<double>(now) / 1024 / 1024,
                                        static_cast<double>(total) / 1024 / 1024,
                                        percent));
    }
}

bool RPCUpdate::IsStarted() const
{
    return started;
}

Object RPCUpdate::GetStatusObject() const
{
    return statusObj;
}

bool RPCUpdate::CheckSha(const std::string& fileName) const
{
    bool result = false;
    std::string newSha = updater.GetDownloadSha256Sum();
    try
    {
        std::string sha = Sha256Sum(fileName);
        if (!sha.empty()) {
            if (newSha.compare(sha) == 0) {
                result = true;
            } else {
                result = false;
            }
        } else {
            result = false;
        }
    } catch(std::runtime_error &e) {
        result = false;
    }
    return result;
}

Value update(const Array& params, bool fHelp)
{
    RPCUpdate rpcUpdate;
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
    if (!fServer)
    {
        throw runtime_error("Command is available only in server mode."
            "\ncrown-qt will automatically check and notify if there is an updates\n");
    }

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
        if (rpcUpdate.IsStarted())
        {
            return "Download is in progress. Run 'crown-cli update status' to check the status.";
        }
        boost::thread t(boost::bind(&RPCUpdate::Download, rpcUpdate));
        return "Crown download started. \nRun 'crown-cli update status' to check download status.";
    }

    if (strCommand == "status")
    {
        return rpcUpdate.GetStatusObject();
    }

    if (strCommand == "install")
    {
        if (!updater.Check())
        {
            return "You are running the latest version of Crown - " + FormatVersion(CLIENT_VERSION);
        }
        if (rpcUpdate.IsStarted())
        {
            return "Download is in progress. Run 'crown-cli update status' to check the status.";
        }
        boost::thread t(boost::bind(&RPCUpdate::Install, rpcUpdate));
        return "Crown install started. \nRun 'crown-cli update status' to check download status.";
    }
    return "";
}
