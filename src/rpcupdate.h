// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_UPDATE_H
#define RPC_UPDATE_H 

#include "updater.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include <boost/filesystem.hpp>

class RPCUpdate
{
public:
    bool Download();
    void Install();
    bool IsStarted() const;
    Object GetStatusObject() const;
    static void ProgressFunction(curl_off_t now, curl_off_t total);

private:
    bool CheckSha(const std::string& fileName) const;
    std::string GetArchivePath() const;

private:
    static Object statusObj;
    static bool started;
    boost::filesystem::path tempDir;
};

#endif // RPC_UPDATE_H
