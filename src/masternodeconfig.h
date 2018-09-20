
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_MASTERNODECONFIG_H_
#define SRC_MASTERNODECONFIG_H_

class CMasternodeConfig;
extern CMasternodeConfig masternodeConfig;

class CMasternodeConfig
{

public:

    class CMasternodeEntry {

    private:
        std::string alias;
        std::string ip;
        std::string privKey;
        std::string txHash;
        std::string outputIndex;
    public:

        CMasternodeEntry(const std::string& alias, const std::string& ip, const std::string& privKey, const std::string& txHash, const std::string& outputIndex) {
            this->alias = alias;
            this->ip = ip;
            this->privKey = privKey;
            this->txHash = txHash;
            this->outputIndex = outputIndex;
        }

        const std::string& getAlias() const {
            return alias;
        }

        const std::string& getOutputIndex() const {
            return outputIndex;
        }

        const std::string& getPrivKey() const {
            return privKey;
        }

        const std::string& getTxHash() const {
            return txHash;
        }

        const std::string& getIp() const {
            return ip;
        }
    };

    CMasternodeConfig() {
        entries = std::vector<CMasternodeEntry>();
    }

    void clear();
    bool read(std::string& strErrRet);
    void add(const std::string& alias, const std::string& ip, const std::string& privKey, const std::string& txHash, const std::string& outputIndex);

    std::vector<CMasternodeEntry>& getEntries() {
        return entries;
    }

    int getCount() {
        return (int)entries.size();
    }

private:
    std::vector<CMasternodeEntry> entries;


};


#endif /* SRC_MASTERNODECONFIG_H_ */
