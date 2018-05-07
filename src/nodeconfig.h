// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_NODECONFIG_H_
#define SRC_NODECONFIG_H_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>

class CNodeEntry {

private:
    std::string alias;
    std::string ip;
    std::string privKey;
    std::string txHash;
    std::string outputIndex;
public:

    CNodeEntry(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
        this->alias = alias;
        this->ip = ip;
        this->privKey = privKey;
        this->txHash = txHash;
        this->outputIndex = outputIndex;
    }

    const std::string& getAlias() const {
        return alias;
    }

    void setAlias(const std::string& alias) {
        this->alias = alias;
    }

    const std::string& getOutputIndex() const {
        return outputIndex;
    }

    void setOutputIndex(const std::string& outputIndex) {
        this->outputIndex = outputIndex;
    }

    const std::string& getPrivKey() const {
        return privKey;
    }

    void setPrivKey(const std::string& privKey) {
        this->privKey = privKey;
    }

    const std::string& getTxHash() const {
        return txHash;
    }

    void setTxHash(const std::string& txHash) {
        this->txHash = txHash;
    }

    const std::string& getIp() const {
        return ip;
    }

    void setIp(const std::string& ip) {
        this->ip = ip;
    }
};

class CNodeConfig
{
public:
    CNodeConfig() {
        entries = std::vector<CNodeEntry>();
    }

    void clear();
    bool read(std::string& strErr);
    bool write();
    bool aliasExists(const std::string& alias);
    void add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex);
    void add(CNodeEntry cne);

    std::vector<CNodeEntry>& getEntries() {
        return entries;
    }

    int getCount() {
        int c = -1;
        BOOST_FOREACH(CNodeEntry e, entries) {
            if(e.getAlias() != "") c++;
        }
        return c;
    }

private:
    virtual boost::filesystem::path getNodeConfigFile() = 0;
    virtual std::string getHeader() = 0;
    virtual std::string getFileName() = 0;
    std::vector<CNodeEntry> entries;
};


#endif /* SRC_NODECONFIG_H_ */
