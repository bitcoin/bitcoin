// Copyright (c) 2014 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _KEEPASS_H_
#define _KEEPASS_H_

#define KEEPASS_CRYPTO_KEY_SIZE 32
#define KEEPASS_CRYPTO_BLOCK_SIZE 16
#define KEEPASS_KEEPASSHTTP_HOST "localhost"
#define KEEPASS_KEEPASSHTTP_PORT 19455

#include <string>
#include <vector>
#include <map>

#include "json/json_spirit_value.h"
#include "allocators.h"

class CKeePassIntegrator {

    bool bIsActive;
    unsigned int nPort;
    SecureString sKeyBase64;
    SecureString sKey;
    SecureString sUrl;
    //SecureString sSubmitUrl;
    std::string sKeePassId;
    std::string sKeePassEntryName;

    class CKeePassRequest {

        json_spirit::Object requestObj;
        std::string sType;
        std::string sIV;
        SecureString sKey;

        void init();

    public:
        void addStrParameter(std::string sName, std::string sValue); // Regular
        void addStrParameter(std::string sName, SecureString sValue); // Encrypt
        std::string getJson();

        CKeePassRequest(SecureString sKey, std::string sType)
        {
            this->sKey = sKey;
            this->sType = sType;
            init();
        };
    };


    class CKeePassEntry {

        SecureString uuid;
        SecureString name;
        SecureString login;
        SecureString password;

    public:
        CKeePassEntry(SecureString uuid, SecureString name, SecureString login, SecureString password) :
            uuid(uuid), name(name), login(login), password(password) {
        }

        SecureString getUuid() {
            return uuid;
        }

        SecureString getName() {
            return name;
        }

        SecureString getLogin() {
            return login;
        }

        SecureString getPassword() {
            return password;
        }

    };


    class CKeePassResponse {

        bool bSuccess;
        std::string sType;
        std::string sIV;
        SecureString sKey;

        void parseResponse(std::string sResponse);

    public:
        json_spirit::Object responseObj;
        CKeePassResponse(SecureString sKey, std::string sResponse) {
            this->sKey = sKey;
            parseResponse(sResponse);
        }

        bool getSuccess() {
            return bSuccess;
        }

        SecureString getSecureStr(std::string sName);
        std::string getStr(std::string sName);
        std::vector<CKeePassEntry> getEntries();

        SecureString decrypt(std::string sValue); // DecodeBase64 and decrypt arbitrary string value

    };

    static SecureString generateRandomKey(size_t nSize);
    static std::string constructHTTPPost(const std::string& strMsg, const std::map<std::string,std::string>& mapRequestHeaders);
    void doHTTPPost(const std::string& sRequest, int& nStatus, std::string& sResponse);
    void rpcTestAssociation(bool bTriggerUnlock);
    std::vector<CKeePassEntry> rpcGetLogins();
    void rpcSetLogin(const SecureString& strWalletPass, const SecureString& sEntryId);

public:
    CKeePassIntegrator();
    void init();
    static SecureString generateKeePassKey();
    void rpcAssociate(std::string& sId, SecureString& sKeyBase64);
    SecureString retrievePassphrase();
    void updatePassphrase(const SecureString& sWalletPassphrase);

};

extern CKeePassIntegrator keePassInt;

#endif
