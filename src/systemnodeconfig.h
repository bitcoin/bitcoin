// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_SYSTEMNODECONFIG_H_
#define SRC_SYSTEMNODECONFIG_H_

#include <string>
#include <vector>
#include "nodeconfig.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>

class CSystemnodeConfig;
extern CSystemnodeConfig systemnodeConfig;

class CSystemnodeConfig : public CNodeConfig
{
private:
    boost::filesystem::path getNodeConfigFile();
    std::string getHeader();
    std::string getFileName();
};

#endif /* SRC_SYSTEMNODECONFIG_H_ */
