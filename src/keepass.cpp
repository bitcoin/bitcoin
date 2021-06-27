// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <keepass.h>

#include <wallet/crypter.h>
#include <clientversion.h>
#include <protocol.h>
#include <random.h>
#include <rpc/protocol.h>

// Necessary to prevent compile errors due to forward declaration of
//CScript in serialize.h (included from crypter.h)
#include <script/script.h>
#include <script/standard.h>

#include <util/system.h>
#include <util/strencodings.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <support/cleanse.h> // for OPENSSL_cleanse()

const char* CKeePassIntegrator::KEEPASS_HTTP_HOST = "localhost";

CKeePassIntegrator keePassInt;

// Base64 decoding with secure memory allocation
SecureString DecodeBase64Secure(const SecureString& sInput)
{
    SecureString output;

    // Init openssl BIO with base64 filter and memory input
    BIO *b64, *mem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    mem = BIO_new_mem_buf((void *) sInput.data(), sInput.size());
    BIO_push(b64, mem);

    // Prepare buffer to receive decoded data
    if(sInput.size() % 4 != 0) {
        throw std::runtime_error("Input length should be a multiple of 4");
    }
    size_t nMaxLen = sInput.size() / 4 * 3; // upper bound, guaranteed divisible by 4
    output.resize(nMaxLen);

    // Decode the string
    size_t nLen;
    nLen = BIO_read(b64, (void *) output.data(), sInput.size());
    output.resize(nLen);

    // Free memory
    BIO_free_all(b64);
    return output;
}

// Base64 encoding with secure memory allocation
SecureString EncodeBase64Secure(const SecureString& sInput)
{
    // Init openssl BIO with base64 filter and memory output
    BIO *b64, *mem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines in output
    mem = BIO_new(BIO_s_mem());
    BIO_push(b64, mem);

    // Decode the string
    BIO_write(b64, sInput.data(), sInput.size());
    (void) BIO_flush(b64);

    // Create output variable from buffer mem ptr
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    SecureString output(bptr->data, bptr->length);

    // Cleanse secure data buffer from memory
    memory_cleanse((void *) bptr->data, bptr->length);

    // Free memory
    BIO_free_all(b64);
    return output;
}

CKeePassIntegrator::CKeePassIntegrator()
    :sKeyBase64(" "), sKey(" "), sUrl(" ") // Prevent LockedPageManagerBase complaints
{
    sKeyBase64.clear(); // Prevent LockedPageManagerBase complaints
    sKey.clear(); // Prevent LockedPageManagerBase complaints
    sUrl.clear(); // Prevent LockedPageManagerBase complaints
    bIsActive = false;
    nPort = DEFAULT_KEEPASS_HTTP_PORT;
}

// Initialze from application context
void CKeePassIntegrator::init()
{
    bIsActive = gArgs.GetBoolArg("-keepass", false);
    nPort = gArgs.GetArg("-keepassport", DEFAULT_KEEPASS_HTTP_PORT);
    sKeyBase64 = SecureString(gArgs.GetArg("-keepasskey", "").c_str());
    strKeePassId = gArgs.GetArg("-keepassid", "");
    strKeePassEntryName = gArgs.GetArg("-keepassname", "");
    // Convert key if available
    if(sKeyBase64.size() > 0)
    {
        sKey = DecodeBase64Secure(sKeyBase64);
    }
    // Construct url if available
    if(strKeePassEntryName.size() > 0)
    {
        sUrl = SecureString("http://");
        sUrl += SecureString(strKeePassEntryName.c_str());
        sUrl += SecureString("/");
    }
}

void CKeePassIntegrator::CKeePassRequest::addStrParameter(const std::string& strName, const std::string& strValue)
{
    requestObj.pushKV(strName, strValue);
}

void CKeePassIntegrator::CKeePassRequest::addStrParameter(const std::string& strName, const SecureString& sValue)
{
    std::string sCipherValue;

    if(!EncryptAES256(sKey, sValue, strIV, sCipherValue))
    {
        throw std::runtime_error("Unable to encrypt Verifier");
    }

    addStrParameter(strName, EncodeBase64(sCipherValue));
}

std::string CKeePassIntegrator::CKeePassRequest::getJson()
{
    return requestObj.write();
}

void CKeePassIntegrator::CKeePassRequest::init()
{
    SecureString sIVSecure = generateRandomKey(KEEPASS_CRYPTO_BLOCK_SIZE);
    strIV = std::string(sIVSecure.data(), sIVSecure.size());
    // Generate Nonce, Verifier and RequestType
    SecureString sNonceBase64Secure = EncodeBase64Secure(sIVSecure);
    addStrParameter("Nonce", std::string(sNonceBase64Secure.data(), sNonceBase64Secure.size())); // Plain
    addStrParameter("Verifier", sNonceBase64Secure); // Encoded
    addStrParameter("RequestType", strType);
}

void CKeePassIntegrator::CKeePassResponse::parseResponse(const std::string& strResponse)
{
    UniValue responseValue;
    if(!responseValue.read(strResponse))
    {
        throw std::runtime_error("Unable to parse KeePassHttp response");
    }

    responseObj = responseValue;

    // retrieve main values
    bSuccess = responseObj["Success"].get_bool();
    strType = getStr("RequestType");
    strIV = DecodeBase64(getStr("Nonce"));
}

std::string CKeePassIntegrator::CKeePassResponse::getStr(const std::string& strName)
{
    return responseObj[strName].get_str();
}

SecureString CKeePassIntegrator::CKeePassResponse::getSecureStr(const std::string& strName)
{
    std::string strValueBase64Encrypted(responseObj[strName].get_str());
    SecureString sValue;
    try
    {
        sValue = decrypt(strValueBase64Encrypted);
    }
    catch (std::exception &e)
    {
        std::string strError = "Exception occured while decrypting ";
        strError += strName + ": " + e.what();
        throw std::runtime_error(strError);
    }
    return sValue;
}

SecureString CKeePassIntegrator::CKeePassResponse::decrypt(const std::string& strValueBase64Encrypted)
{
    std::string strValueEncrypted = DecodeBase64(strValueBase64Encrypted);
    SecureString sValue;
    if(!DecryptAES256(sKey, strValueEncrypted, strIV, sValue))
    {
      throw std::runtime_error("Unable to decrypt value.");
    }
    return sValue;
}

std::vector<CKeePassIntegrator::CKeePassEntry> CKeePassIntegrator::CKeePassResponse::getEntries()
{

    std::vector<CKeePassEntry> vEntries;

    UniValue aEntries = responseObj["Entries"].get_array();
    for(size_t i = 0; i < aEntries.size(); i++)
    {
        SecureString sEntryUuid(decrypt(aEntries[i]["Uuid"].get_str().c_str()));
        SecureString sEntryName(decrypt(aEntries[i]["Name"].get_str().c_str()));
        SecureString sEntryLogin(decrypt(aEntries[i]["Login"].get_str().c_str()));
        SecureString sEntryPassword(decrypt(aEntries[i]["Password"].get_str().c_str()));
        CKeePassEntry entry(sEntryUuid, sEntryName, sEntryLogin, sEntryPassword);
        vEntries.push_back(entry);
    }

    return vEntries;

}

SecureString CKeePassIntegrator::generateRandomKey(size_t nSize)
{
    // Generates random key
    SecureString sKey;
    sKey.resize(nSize);

    GetStrongRandBytes((unsigned char *) sKey.data(), nSize);

    return sKey;
}

// Construct POST body for RPC JSON call
std::string CKeePassIntegrator::constructHTTPPost(const std::string& strMsg, const std::map<std::string,std::string>& mapRequestHeaders)
{
    std::ostringstream streamOut;
    streamOut << "POST / HTTP/1.1\r\n"
      << "User-Agent: dash-json-rpc/" << FormatFullVersion() << "\r\n"
      << "Host: localhost\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << strMsg.size() << "\r\n"
      << "Connection: close\r\n"
      << "Accept: application/json\r\n";
    for (const auto& item : mapRequestHeaders)
        streamOut << item.first << ": " << item.second << "\r\n";
    streamOut << "\r\n" << strMsg;

    return streamOut.str();
}

/** Reply structure for request_done to fill in */
struct HTTPReply
{
    int nStatus;
    std::string strBody;
};

static void http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);

    if (req == nullptr) {
        /* If req is nullptr, it means an error occurred while connecting, but
         * I'm not sure how to find out which one. We also don't really care.
         */
        reply->nStatus = 0;
        return;
    }

    reply->nStatus = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->strBody = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

// Send RPC message to KeePassHttp
void CKeePassIntegrator::doHTTPPost(const std::string& sRequest, int& nStatusRet, std::string& strResponseRet)
{
    // Create event base
    struct event_base *base = event_base_new(); // TODO RAII
    if (!base)
        throw std::runtime_error("cannot create event_base");

    // Synchronously look up hostname
    struct evhttp_connection *evcon = evhttp_connection_base_new(base, nullptr, KEEPASS_HTTP_HOST, DEFAULT_KEEPASS_HTTP_PORT); // TODO RAII
    if (evcon == nullptr)
        throw std::runtime_error("create connection failed");
    evhttp_connection_set_timeout(evcon, KEEPASS_HTTP_CONNECT_TIMEOUT);

    HTTPReply response;
    struct evhttp_request *req = evhttp_request_new(http_request_done, (void*)&response); // TODO RAII
    if (req == nullptr)
        throw std::runtime_error("create http request failed");

    struct evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    assert(output_headers);
    evhttp_add_header(output_headers, "User-Agent", ("dash-json-rpc/" + FormatFullVersion()).c_str());
    evhttp_add_header(output_headers, "Host", KEEPASS_HTTP_HOST);
    evhttp_add_header(output_headers, "Accept", "application/json");
    evhttp_add_header(output_headers, "Content-Type", "application/json");
    evhttp_add_header(output_headers, "Connection", "close");

    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::doHTTPPost -- send POST data\n");

    struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
    assert(output_buffer);
    evbuffer_add(output_buffer, sRequest.data(), sRequest.size());

    int r = evhttp_make_request(evcon, req, EVHTTP_REQ_POST, "/");
    if (r != 0) {
        evhttp_connection_free(evcon);
        event_base_free(base);
        throw std::runtime_error("send http request failed");
    }

    event_base_dispatch(base);
    evhttp_connection_free(evcon);
    event_base_free(base);

    nStatusRet = response.nStatus;
    if (response.nStatus == 0)
        throw std::runtime_error("couldn't connect to server");
    else if (response.nStatus >= 400 && response.nStatus != HTTP_BAD_REQUEST && response.nStatus != HTTP_NOT_FOUND && response.nStatus != HTTP_INTERNAL_SERVER_ERROR)
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.nStatus));
    else if (response.strBody.empty())
        throw std::runtime_error("no response from server");

    // Parse reply
    UniValue valReply(UniValue::VSTR);
    if (!valReply.read(response.strBody))
         throw std::runtime_error("couldn't parse reply from server");
    const UniValue& reply = valReply.get_obj();
    if (reply.empty())
        throw std::runtime_error("expected reply to have result, error and id properties");

    strResponseRet = valReply.get_str();
}

void CKeePassIntegrator::rpcTestAssociation(bool bTriggerUnlock)
{
    CKeePassRequest request(sKey, "test-associate");
    request.addStrParameter("TriggerUnlock", std::string(bTriggerUnlock ? "true" : "false"));

    int nStatus;
    std::string strResponse;

    doHTTPPost(request.getJson(), nStatus, strResponse);

    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::rpcTestAssociation -- send result: status: %d response: %s\n", nStatus, strResponse);
}

std::vector<CKeePassIntegrator::CKeePassEntry> CKeePassIntegrator::rpcGetLogins()
{

    // Convert key format
    SecureString sKey = DecodeBase64Secure(sKeyBase64);

    CKeePassRequest request(sKey, "get-logins");
    request.addStrParameter("addStrParameter", std::string("true"));
    request.addStrParameter("TriggerUnlock", std::string("true"));
    request.addStrParameter("Id", strKeePassId);
    request.addStrParameter("Url", sUrl);

    int nStatus;
    std::string strResponse;

    doHTTPPost(request.getJson(), nStatus, strResponse);

    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::rpcGetLogins -- send result: status: %d\n", nStatus);

    if(nStatus != 200)
    {
        std::string strError = "Error returned by KeePassHttp: HTTP code ";
        strError += itostr(nStatus);
        strError += " - Response: ";
        strError += " response: [";
        strError += strResponse;
        strError += "]";
        throw std::runtime_error(strError);
    }

    // Parse the response
    CKeePassResponse response(sKey, strResponse);

    if(!response.getSuccess())
    {
        std::string strError = "KeePassHttp returned failure status";
        throw std::runtime_error(strError);
    }

    return response.getEntries();
}

void CKeePassIntegrator::rpcSetLogin(const SecureString& sWalletPass, const SecureString& sEntryId)
{

    // Convert key format
    SecureString sKey = DecodeBase64Secure(sKeyBase64);

    CKeePassRequest request(sKey, "set-login");
    request.addStrParameter("Id", strKeePassId);
    request.addStrParameter("Url", sUrl);

    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::rpcSetLogin -- send Url: %s\n", sUrl);

    //request.addStrParameter("SubmitUrl", sSubmitUrl); // Is used to construct the entry title
    request.addStrParameter("Login", SecureString("dash"));
    request.addStrParameter("Password", sWalletPass);
    if(sEntryId.size() != 0)
    {
        request.addStrParameter("Uuid", sEntryId); // Update existing
    }

    int nStatus;
    std::string strResponse;

    doHTTPPost(request.getJson(), nStatus, strResponse);


    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::rpcSetLogin -- send result: status: %d response: %s\n", nStatus, strResponse);

    if(nStatus != 200)
    {
        std::string strError = "Error returned: HTTP code ";
        strError += itostr(nStatus);
        strError += " - Response: ";
        strError += " response: [";
        strError += strResponse;
        strError += "]";
        throw std::runtime_error(strError);
    }

    // Parse the response
    CKeePassResponse response(sKey, strResponse);

    if(!response.getSuccess())
    {
        throw std::runtime_error("KeePassHttp returned failure status");
    }
}


SecureString CKeePassIntegrator::generateKeePassKey()
{
    SecureString sKey = generateRandomKey(KEEPASS_CRYPTO_KEY_SIZE);
    SecureString sKeyBase64 = EncodeBase64Secure(sKey);
    return sKeyBase64;
}

void CKeePassIntegrator::rpcAssociate(std::string& strIdRet, SecureString& sKeyBase64Ret)
{
    sKey = generateRandomKey(KEEPASS_CRYPTO_KEY_SIZE);
    CKeePassRequest request(sKey, "associate");

    sKeyBase64Ret = EncodeBase64Secure(sKey);
    request.addStrParameter("Key", std::string(sKeyBase64Ret.data(), sKeyBase64Ret.size()));

    int nStatus;
    std::string strResponse;

    doHTTPPost(request.getJson(), nStatus, strResponse);

    LogPrint(BCLog::KEEPASS, "CKeePassIntegrator::rpcAssociate -- send result: status: %d response: %s\n", nStatus, strResponse);

    if(nStatus != 200)
    {
        std::string strError = "Error returned: HTTP code ";
        strError += itostr(nStatus);
        strError += " - Response: ";
        strError += " response: [";
        strError += strResponse;
        strError += "]";
        throw std::runtime_error(strError);
    }

    // Parse the response
    CKeePassResponse response(sKey, strResponse);

    if(!response.getSuccess())
    {
        throw std::runtime_error("KeePassHttp returned failure status");
    }

    // If we got here, we were successful. Return the information
    strIdRet = response.getStr("Id");
}

// Retrieve wallet passphrase from KeePass
SecureString CKeePassIntegrator::retrievePassphrase()
{

    // Check we have all required information
    if(sKey.size() == 0)
    {
        throw std::runtime_error("keepasskey parameter is not defined. Please specify the configuration parameter.");
    }
    if(strKeePassId.size() == 0)
    {
        throw std::runtime_error("keepassid parameter is not defined. Please specify the configuration parameter.");
    }
    if(strKeePassEntryName == "")
    {
        throw std::runtime_error("keepassname parameter is not defined. Please specify the configuration parameter.");
    }

    // Retrieve matching logins from KeePass
    std::vector<CKeePassIntegrator::CKeePassEntry> vecEntries = rpcGetLogins();

    // Only accept one unique match
    if(vecEntries.size() == 0)
    {
        throw std::runtime_error("KeePassHttp returned 0 matches, please verify the keepassurl setting.");
    }
    if(vecEntries.size() > 1)
    {
        throw std::runtime_error("KeePassHttp returned multiple matches, bailing out.");
    }

    return vecEntries[0].getPassword();
}

// Update wallet passphrase in keepass
void CKeePassIntegrator::updatePassphrase(const SecureString& sWalletPassphrase)
{
    // Check we have all required information
    if(sKey.size() == 0)
    {
        throw std::runtime_error("keepasskey parameter is not defined. Please specify the configuration parameter.");
    }
    if(strKeePassId.size() == 0)
    {
        throw std::runtime_error("keepassid parameter is not defined. Please specify the configuration parameter.");
    }
    if(strKeePassEntryName == "")
    {
        throw std::runtime_error("keepassname parameter is not defined. Please specify the configuration parameter.");
    }

    SecureString sEntryId("");

    std::string strError;

    // Lookup existing entry
    std::vector<CKeePassIntegrator::CKeePassEntry> vecEntries = rpcGetLogins();

    if(vecEntries.size() > 1)
    {
        throw std::runtime_error("KeePassHttp returned multiple matches, bailing out.");
    }

    if(vecEntries.size() == 1)
    {
        sEntryId = vecEntries[0].getUuid();
    }

    // Update wallet passphrase in KeePass
    rpcSetLogin(sWalletPassphrase, sEntryId);
}
