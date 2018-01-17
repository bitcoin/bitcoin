
// Copyright (c) 2014-2017 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef UPDATER_H
#define UPDATER_H 

#include <iostream>
#define CURL_STATICLIB
#include "curl/curl.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_reader.h"
using namespace json_spirit;

class UpdateInfo;
class Updater;

extern Updater updater;

class Updater
{
public:
    enum OS {
        UNKNOWN,
        LINUX_32,
        LINUX_64,
        WINDOWS_32,
        WINDOWS_64,
        MAC_OS,
    };
    Updater();
    bool Check();
    int GetVersion()
    {
        return version;
    }
    Updater::OS GetOS()
    {
        return os;
    }
    void DownloadFile(std::string url, std::string fileName, void(progressFunction)(curl_off_t, curl_off_t));
    void DownloadFileAsync(std::string url, std::string fileName, void(progressFunction)(curl_off_t, curl_off_t));
    void StopDownload();
    std::string GetDownloadUrl(Updater::OS version = UNKNOWN);
    std::string GetDownloadSha256Sum(Updater::OS version = UNKNOWN);
    std::string GetOsString(Updater::OS os = UNKNOWN);
    bool GetStopDownload()
    {
        return stopDownload;
    }
private:
    std::string updaterInfoUrl;
    bool LoadUpdateInfo();
    Value ParseJson(std::string info);
    void SetOS();
    bool NeedToBeUpdated();
    int GetVersionFromJson();
    std::string GetUrl(Value value);
    std::string GetSha256sum(Value value);

    OS os;
    bool status;
    int version;
    bool stopDownload;
    Value jsonData;
};

#endif
