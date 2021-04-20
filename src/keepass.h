// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEEPASS_H
#define BITCOIN_KEEPASS_H

#include <support/allocators/secure.h>

#include <univalue.h>

class CKeePassIntegrator;

static const unsigned int DEFAULT_KEEPASS_HTTP_PORT     = 19455;

extern CKeePassIntegrator keePassInt;

class CKeePassIntegrator {
private:
    static const int KEEPASS_CRYPTO_KEY_SIZE            = 32;
    static const int KEEPASS_CRYPTO_BLOCK_SIZE          = 16;
    static const int KEEPASS_HTTP_CONNECT_TIMEOUT       = 30;
    static const char* KEEPASS_HTTP_HOST;

    bool bIsActive;
    unsigned int nPort;
    SecureString sKeyBase64;
    SecureString sKey;
    SecureString sUrl;
    //SecureString sSubmitUrl;
    std::string strKeePassId;
    std::string strKeePassEntryName;

    class CKeePassRequest {

        UniValue requestObj;
        std::string strType;
        std::string strIV;
        SecureString sKey;

        void init();

    public:
        void addStrParameter(const std::string& strName, const std::string& strValue); // Regular
        void addStrParameter(const std::string& strName, const SecureString& sValue); // Encrypt
        std::string getJson();

        CKeePassRequest(const SecureString& sKey, const std::string& strType)
        {
            this->sKey = sKey;
            this->strType = strType;
            init();
        };
    };


    class CKeePassEntry {

        SecureString sUuid;
        SecureString sName;
        SecureString sLogin;
        SecureString sPassword;

    public:
        CKeePassEntry(const SecureString& sUuid, const SecureString& sName, const SecureString& sLogin, const SecureString& sPassword) :
            sUuid(sUuid), sName(sName), sLogin(sLogin), sPassword(sPassword) {
        }

        SecureString getUuid() {
            return sUuid;
        }

        SecureString getName() {
            return sName;
        }

        SecureString getLogin() {
            return sLogin;
        }

        SecureString getPassword() {
            return sPassword;
        }

    };


    class CKeePassResponse {

        bool bSuccess;
        std::string strType;
        std::string strIV;
        SecureString sKey;

        void parseResponse(const std::string& strResponse);

    public:
        UniValue responseObj;
        CKeePassResponse(const SecureString& sKey, const std::string& strResponse) {
            this->sKey = sKey;
            parseResponse(strResponse);
        }

        bool getSuccess() {
            return bSuccess;
        }

        SecureString getSecureStr(const std::string& strName);
        std::string getStr(const std::string& strName);
        std::vector<CKeePassEntry> getEntries();

        SecureString decrypt(const std::string& strValue); // DecodeBase64 and decrypt arbitrary string value

    };

    static SecureString generateRandomKey(size_t nSize);
    static std::string constructHTTPPost(const std::string& strMsg, const std::map<std::string,std::string>& mapRequestHeaders);
    void doHTTPPost(const std::string& strRequest, int& nStatusRet, std::string& strResponseRet);
    void rpcTestAssociation(bool bTriggerUnlock);
    std::vector<CKeePassEntry> rpcGetLogins();
    void rpcSetLogin(const SecureString& sWalletPass, const SecureString& sEntryId);

public:
    CKeePassIntegrator();
    void init();
    static SecureString generateKeePassKey();
    void rpcAssociate(std::string& strIdRet, SecureString& sKeyBase64Ret);
    SecureString retrievePassphrase();
    void updatePassphrase(const SecureString& sWalletPassphrase);

};

#endif // BITCOIN_KEEPASS_H