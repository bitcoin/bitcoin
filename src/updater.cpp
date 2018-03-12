// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "updater.h"
#include "clientversion.h"
#include "util.h"
#include "chainparams.h"
#include "rpcprotocol.h"

#include <stdio.h>
#include <curl/curl.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

struct DownloadProgress {
    double lastruntime;
    CURL *curl;
    void(*progressFunction)(curl_off_t, curl_off_t);
};

// Global updater instance
Updater updater;

Updater::Updater() :
    os(DetectOS()),
    status(false),
    version(-1),
    stopDownload(false),
    url("https://raw.githubusercontent.com/Crowndev/crowncoin/master/update.json")
{
}

Updater::OS Updater::DetectOS()
{
    OS os = Updater::LINUX_32;
    #ifdef _WIN32
        os = Updater::WINDOWS_32;
        #ifdef _WIN64
            os = Updater::WINDOWS_64;
        #endif
    #elif __APPLE__
        os = Updater::MAC_OS;
    #elif __linux__
        os = Updater::LINUX_32;
        #if defined(__i386__) || defined(__i686__) || defined(__i486__) || defined(__i586__)
            os = Updater::LINUX_32;
        #elif defined(__x86_64__) || defined(__amd64__)
            os = Updater::LINUX_64;
        #endif
    #endif
    return os;
}

size_t GetUpdateData(const char *data, size_t size, size_t nmemb, std::string *updateData) {
    size_t len = size * nmemb;

    if (updateData != NULL)
    {
        // Append the data to the buffer
        updateData->append(data, len);
    }

    return len;
}

Value Updater::ParseJson(std::string info)
{
    Value value;
    json_spirit::read_string(info, value);
    return value;
}


bool Updater::NeedToBeUpdated()
{
    if (jsonData.type() == obj_type)
    {
        Value version = find_value(jsonData.get_obj(), "NeedToBeUpdated");
        if (version.type() == bool_type)
        {
            return version.get_bool();
        }
    }
    return false;
}

void Updater::SetJsonPath()
{
    updaterInfoUrl = url;
    if (mapArgs.count("-updateurl"))
    {
        updaterInfoUrl = mapArgs["-updateurl"];
    }
}

void Updater::LoadUpdateInfo()
{
    CURL *curl;
    std::string updateData;
    SetJsonPath();
    CURLcode res = CURLE_FAILED_INIT;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, updaterInfoUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetUpdateData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &updateData);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == HTTP_OK)
            {
                CheckAndUpdateStatus(updateData);
            }
            else
            {
                LogPrintf("Updater::GetUpdateInfo() - Error! Server response code - %d\n", response_code);
                throw std::runtime_error(strprintf("Error! Failed to get update information. \nServer response code - %d\n", response_code));
            }
        }
        else
        {
            LogPrintf("Updater::GetUpdateInfo() - Error! Couldn't get data json. Error code - %d\n", res);
            throw std::runtime_error(strprintf("Error! Couldn't get data json. Error code - %d\n", res));
        }
        curl_easy_cleanup(curl);
    }
}

std::string Updater::GetOsString(boost::optional<OS> os)
{
    if (!os) {
        return GetOsString(GetOS());
    }
    switch(os.get()) {
        case Updater::LINUX_32:
            return "Linux32";
        case Updater::LINUX_64:
            return "Linux64";
        case Updater::WINDOWS_32:
            return "Win32";
        case Updater::WINDOWS_64:
            return "Win64";
        case Updater::MAC_OS:
            return "Osx";
        default:
            assert(false);
    }
    return "";
}

std::string Updater::GetUrl(const Value& value)
{
    if (value.type() == obj_type)
    {
        Value urlValue = find_value(value.get_obj(), "Url");
        if (urlValue.type() == str_type)
        {
            return urlValue.get_str();
        }
    }
    return "";
}

std::string Updater::GetSha256sum(Value value)
{
    if (value.type() == obj_type)
    {
        Value sha256sumValue = find_value(value.get_obj(), "sha256sum");
        if (sha256sumValue.type() == str_type)
        {
            return sha256sumValue.get_str();
        }
    }
    return "";
}

int Updater::GetVersionFromJson()
{
    if (jsonData.type() == obj_type)
    {
        Value version = find_value(jsonData.get_obj(), "Version");
        if (version.type() == int_type)
        {
            return version.get_int();
        }
    }
    return -1;
}

std::string Updater::GetDownloadUrl(boost::optional<OS> version)
{
    Value json = find_value(jsonData.get_obj(), GetOsString(version));
    return GetUrl(json);
}

std::string Updater::GetDownloadSha256Sum(boost::optional<OS> version)
{
    Value json = find_value(jsonData.get_obj(), GetOsString(version));
    return GetSha256sum(json);
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, const void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

static int xferinfo(void *p,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow)
{
    struct DownloadProgress *myp = (struct DownloadProgress*)p;
    myp->progressFunction(dlnow, dltotal);
    return updater.GetStopDownload();
}

void Updater::DownloadFileAsync(std::string url, std::string fileName, void(progressFunction)(curl_off_t, curl_off_t))
{
    boost::thread t(boost::bind(&Updater::DownloadFile, this, url, fileName, progressFunction));
}

CURLcode Updater::DownloadFile(std::string url, std::string fileName, void(progressFunction)(curl_off_t, curl_off_t))
{
    stopDownload = false;
    CURL *curl_handle;
    struct DownloadProgress prog;
    prog.progressFunction = progressFunction;
    FILE *pagefile;
    CURLcode res = CURLE_FAILED_INIT;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, &prog);
    pagefile = fopen(fileName.c_str(), "wb");
    if(pagefile) {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
        res = curl_easy_perform(curl_handle);
        fclose(pagefile);
    }
    curl_easy_cleanup(curl_handle);

    if (res != CURLE_OK)
    {
        LogPrintf("Updater::DownloadFile() - Error! Failed to download file - %s. Error code - %d\n", url, res);
    }
    return res;
}

void Updater::StopDownload()
{
    stopDownload = true;
}

void Updater::CheckAndUpdateStatus(const std::string& updateData)
{
    jsonData = ParseJson(updateData);
    version = GetVersionFromJson();
    LogPrintf("Updater::GetUpdateInfo() - Got version from json: %d\n", version);
    if (version > CLIENT_VERSION && NeedToBeUpdated())
    {
        LogPrintf("Updater::GetUpdateInfo() - Version is old\n");
        // There is an update
        status = true;
    }
}

bool Updater::GetStatus()
{
    LoadUpdateInfo();
    return status;
}
