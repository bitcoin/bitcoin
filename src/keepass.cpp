// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keepass.h"

#include "wallet/crypter.h"
#include "clientversion.h"
#include "protocol.h"
#include "random.h"
#include "rpcprotocol.h"

// Necessary to prevent compile errors due to forward declaration of
//CScript in serialize.h (included from crypter.h)
#include "script/script.h"
#include "script/standard.h"

#include "util.h"
#include "utilstrencodings.h"

#include <boost/foreach.hpp>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "support/cleanse.h" // for OPENSSL_cleanse()


CKeePassIntegrator keePassInt;

// Base64 decoding with secure memory allocation
SecureString DecodeBase64Secure(const SecureString& input)
{
    SecureString output;

    // Init openssl BIO with base64 filter and memory input
    BIO *b64, *mem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    mem = BIO_new_mem_buf((void *) &input[0], input.size());
    BIO_push(b64, mem);

    // Prepare buffer to receive decoded data
    if(input.size() % 4 != 0) {
        throw std::runtime_error("Input length should be a multiple of 4");
    }
    size_t nMaxLen = input.size() / 4 * 3; // upper bound, guaranteed divisible by 4
    output.resize(nMaxLen);

    // Decode the string
    size_t nLen;
    nLen = BIO_read(b64, (void *) &output[0], input.size());
    output.resize(nLen);

    // Free memory
    BIO_free_all(b64);
    return output;
}

// Base64 encoding with secure memory allocation
SecureString EncodeBase64Secure(const SecureString& input)
{
    // Init openssl BIO with base64 filter and memory output
    BIO *b64, *mem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines in output
    mem = BIO_new(BIO_s_mem());
    BIO_push(b64, mem);

    // Decode the string
    BIO_write(b64, &input[0], input.size());
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
    bIsActive = GetBoolArg("-keepass", false);
    nPort = GetArg("-keepassport", DEFAULT_KEEPASS_HTTP_PORT);
    sKeyBase64 = SecureString(GetArg("-keepasskey", "").c_str());
    sKeePassId = GetArg("-keepassid", "");
    sKeePassEntryName = GetArg("-keepassname", "");
    // Convert key if available
    if(sKeyBase64.size() > 0)
    {
        sKey = DecodeBase64Secure(sKeyBase64);
    }
    // Construct url if available
    if(sKeePassEntryName.size() > 0)
    {
        sUrl = SecureString("http://");
        sUrl += SecureString(sKeePassEntryName.c_str());
        sUrl += SecureString("/");
        //sSubmitUrl = "http://";
        //sSubmitUrl += SecureString(sKeePassEntryName.c_str());
    }
}

void CKeePassIntegrator::CKeePassRequest::addStrParameter(std::string sName, std::string sValue)
{
    requestObj.push_back(Pair(sName, sValue));
}

void CKeePassIntegrator::CKeePassRequest::addStrParameter(std::string sName, SecureString sValue)
{
    std::string sCipherValue;

    if(!EncryptAES256(sKey, sValue, sIV, sCipherValue))
    {
        throw std::runtime_error("Unable to encrypt Verifier");
    }

    addStrParameter(sName, EncodeBase64(sCipherValue));
}

std::string CKeePassIntegrator::CKeePassRequest::getJson()
{
    return requestObj.write();
}

void CKeePassIntegrator::CKeePassRequest::init()
{
    SecureString sIVSecure = generateRandomKey(KEEPASS_CRYPTO_BLOCK_SIZE);
    sIV = std::string(&sIVSecure[0], sIVSecure.size());
    // Generate Nonce, Verifier and RequestType
    SecureString sNonceBase64Secure = EncodeBase64Secure(sIVSecure);
    addStrParameter("Nonce", std::string(&sNonceBase64Secure[0], sNonceBase64Secure.size())); // Plain
    addStrParameter("Verifier", sNonceBase64Secure); // Encoded
    addStrParameter("RequestType", sType);
}

void CKeePassIntegrator::CKeePassResponse::parseResponse(std::string sResponse)
{
    UniValue responseValue;
    if(!responseValue.read(sResponse))
    {
        throw std::runtime_error("Unable to parse KeePassHttp response");
    }

    responseObj = responseValue;

    // retrieve main values
    bSuccess = responseObj["Success"].get_bool();
    sType = getStr("RequestType");
    sIV = DecodeBase64(getStr("Nonce"));
}

std::string CKeePassIntegrator::CKeePassResponse::getStr(std::string sName)
{
    return responseObj[sName].get_str();
}

SecureString CKeePassIntegrator::CKeePassResponse::getSecureStr(std::string sName)
{
    std::string sValueBase64Encrypted(responseObj[sName].get_str());
    SecureString sValue;
    try
    {
        sValue = decrypt(sValueBase64Encrypted);
    }
    catch (std::exception &e)
    {
        std::string sErrorMessage = "Exception occured while decrypting ";
        sErrorMessage += sName + ": " + e.what();
        throw std::runtime_error(sErrorMessage);
    }
    return sValue;
}

SecureString CKeePassIntegrator::CKeePassResponse::decrypt(std::string sValueBase64Encrypted)
{
    std::string sValueEncrypted = DecodeBase64(sValueBase64Encrypted);
    SecureString sValue;
    if(!DecryptAES256(sKey, sValueEncrypted, sIV, sValue))
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
    SecureString key;
    key.resize(nSize);

    RandAddSeedPerfmon();
    GetRandBytes((unsigned char *) &key[0], nSize);

    return key;
}

// Construct POST body for RPC JSON call
std::string CKeePassIntegrator::constructHTTPPost(const std::string& strMsg, const std::map<std::string,std::string>& mapRequestHeaders)
{
    std::ostringstream s;
    s << "POST / HTTP/1.1\r\n"
      << "User-Agent: dash-json-rpc/" << FormatFullVersion() << "\r\n"
      << "Host: localhost\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << strMsg.size() << "\r\n"
      << "Connection: close\r\n"
      << "Accept: application/json\r\n";
    BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item, mapRequestHeaders)
        s << item.first << ": " << item.second << "\r\n";
    s << "\r\n" << strMsg;

    return s.str();
}

/** Reply structure for request_done to fill in */
struct HTTPReply
{
    int status;
    std::string body;
};

static void http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);

    if (req == NULL) {
        /* If req is NULL, it means an error occurred while connecting, but
         * I'm not sure how to find out which one. We also don't really care.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

// Send RPC message to KeePassHttp
void CKeePassIntegrator::doHTTPPost(const std::string& sRequest, int& nStatus, std::string& sResponse)
{
//    // Prepare communication
//    boost::asio::io_service io_service;

//    // Get a list of endpoints corresponding to the server name.
//    tcp::resolver resolver(io_service);
//    tcp::resolver::query query(KEEPASS_HTTP_HOST, boost::lexical_cast<std::string>(nPort));
//    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
//    tcp::resolver::iterator end;

//    // Try each endpoint until we successfully establish a connection.
//    tcp::socket socket(io_service);
//    boost::system::error_code error = boost::asio::error::host_not_found;
//    while (error && endpoint_iterator != end)
//    {
//      socket.close();
//      socket.connect(*endpoint_iterator++, error);
//    }

//    if(error)
//    {
//        throw boost::system::system_error(error);
//    }
    // Create event base
    struct event_base *base = event_base_new(); // TODO RAII
    if (!base)
        throw std::runtime_error("cannot create event_base");

    // Synchronously look up hostname
    struct evhttp_connection *evcon = evhttp_connection_base_new(base, NULL, KEEPASS_HTTP_HOST, DEFAULT_KEEPASS_HTTP_PORT); // TODO RAII
    if (evcon == NULL)
        throw std::runtime_error("create connection failed");
    evhttp_connection_set_timeout(evcon, KEEPASS_HTTP_CONNECT_TIMEOUT);

    // Form the request.
//    std::map<std::string, std::string> mapRequestHeaders;
//    std::string strPost = constructHTTPPost(sRequest, mapRequestHeaders);

    HTTPReply response;
    struct evhttp_request *req = evhttp_request_new(http_request_done, (void*)&response); // TODO RAII
    if (req == NULL)
        throw std::runtime_error("create http request failed");

    struct evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    assert(output_headers);
//    s << "POST / HTTP/1.1\r\n"
    evhttp_add_header(output_headers, "User-Agent", ("dash-json-rpc/" + FormatFullVersion()).c_str());
    evhttp_add_header(output_headers, "Host", KEEPASS_HTTP_HOST);
    evhttp_add_header(output_headers, "Accept", "application/json");
    evhttp_add_header(output_headers, "Content-Type", "application/json");
//    evhttp_add_header(output_headers, "Content-Length", itostr(strMsg.size()).c_str());
    evhttp_add_header(output_headers, "Connection", "close");

    // Logging of actual post data disabled as to not write passphrase in debug.log. Only enable temporarily when needed
    //LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- send POST data: %s\n", strPost);
    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- send POST data\n");

//    boost::asio::streambuf request;
//    std::ostream request_stream(&request);
//    request_stream << strPost;

//    // Send the request.
//    boost::asio::write(socket, request);

//    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- request written\n");

//    // Read the response status line. The response streambuf will automatically
//    // grow to accommodate the entire line. The growth may be limited by passing
//    // a maximum size to the streambuf constructor.
//    boost::asio::streambuf response;
//    boost::asio::read_until(socket, response, "\r\n");

//    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- request status line read\n");

//    // Receive HTTP reply status
//    int nProto = 0;
//    std::istream response_stream(&response);
//    nStatus = ReadHTTPStatus(response_stream, nProto);

    // Attach request data
//    std::string sRequest = JSONRPCRequest(strMethod, params, 1);
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

//    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- reading response body start\n");
//    // Read until EOF, writing data to output as we go.
//    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
//    {
//        if (error != boost::asio::error::eof)
//        {
//            if (error != 0)
//            { // 0 is success
//                throw boost::system::system_error(error);
//            }
//        }
//    }
//    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- reading response body end\n");
//
//    // Receive HTTP reply message headers and body
//    std::map<std::string, std::string> mapHeaders;
//    ReadHTTPMessage(response_stream, mapHeaders, sResponse, nProto, std::numeric_limits<size_t>::max());
//    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost -- Processed body\n");

    nStatus = response.status;
    if (response.status == 0)
        throw std::runtime_error("couldn't connect to server");
    else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST && response.status != HTTP_NOT_FOUND && response.status != HTTP_INTERNAL_SERVER_ERROR)
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.status));
    else if (response.body.empty())
        throw std::runtime_error("no response from server");

    // Parse reply
    UniValue valReply(UniValue::VSTR);
    if (!valReply.read(response.body))
         throw std::runtime_error("couldn't parse reply from server");
    const UniValue& reply = valReply.get_obj();
    if (reply.empty())
        throw std::runtime_error("expected reply to have result, error and id properties");

    sResponse = valReply.get_str();
}

void CKeePassIntegrator::rpcTestAssociation(bool bTriggerUnlock)
{
    CKeePassRequest request(sKey, "test-associate");
    request.addStrParameter("TriggerUnlock", std::string(bTriggerUnlock ? "true" : "false"));

    int nStatus;
    std::string sResponse;

    doHTTPPost(request.getJson(), nStatus, sResponse);

    LogPrint("keepass", "CKeePassIntegrator::rpcTestAssociation -- send result: status: %d response: %s\n", nStatus, sResponse);
}

std::vector<CKeePassIntegrator::CKeePassEntry> CKeePassIntegrator::rpcGetLogins()
{

    // Convert key format
    SecureString sKey = DecodeBase64Secure(sKeyBase64);

    CKeePassRequest request(sKey, "get-logins");
    request.addStrParameter("addStrParameter", std::string("true"));
    request.addStrParameter("TriggerUnlock", std::string("true"));
    request.addStrParameter("Id", sKeePassId);
    request.addStrParameter("Url", sUrl);

    int nStatus;
    std::string sResponse;

    doHTTPPost(request.getJson(), nStatus, sResponse);

    // Logging of actual response data disabled as to not write passphrase in debug.log. Only enable temporarily when needed
    //LogPrint("keepass", "CKeePassIntegrator::rpcGetLogins -- send result: status: %d response: %s\n", nStatus, sResponse);
    LogPrint("keepass", "CKeePassIntegrator::rpcGetLogins -- send result: status: %d\n", nStatus);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned by KeePassHttp: HTTP code ";
        sErrorMessage += itostr(nStatus);
        sErrorMessage += " - Response: ";
        sErrorMessage += " response: [";
        sErrorMessage += sResponse;
        sErrorMessage += "]";
        throw std::runtime_error(sErrorMessage);
    }

    // Parse the response
    CKeePassResponse response(sKey, sResponse);

    if(!response.getSuccess())
    {
        std::string sErrorMessage = "KeePassHttp returned failure status";
        throw std::runtime_error(sErrorMessage);
    }

    return response.getEntries();
}

void CKeePassIntegrator::rpcSetLogin(const SecureString& strWalletPass, const SecureString& sEntryId)
{

    // Convert key format
    SecureString sKey = DecodeBase64Secure(sKeyBase64);

    CKeePassRequest request(sKey, "set-login");
    request.addStrParameter("Id", sKeePassId);
    request.addStrParameter("Url", sUrl);

    LogPrint("keepass", "CKeePassIntegrator::rpcSetLogin -- send Url: %s\n", sUrl);

    //request.addStrParameter("SubmitUrl", sSubmitUrl); // Is used to construct the entry title
    request.addStrParameter("Login", SecureString("dash"));
    request.addStrParameter("Password", strWalletPass);
    if(sEntryId.size() != 0)
    {
        request.addStrParameter("Uuid", sEntryId); // Update existing
    }

    int nStatus;
    std::string sResponse;

    doHTTPPost(request.getJson(), nStatus, sResponse);


    LogPrint("keepass", "CKeePassIntegrator::rpcSetLogin -- send result: status: %d response: %s\n", nStatus, sResponse);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned: HTTP code ";
        sErrorMessage += itostr(nStatus);
        sErrorMessage += " - Response: ";
        sErrorMessage += " response: [";
        sErrorMessage += sResponse;
        sErrorMessage += "]";
        throw std::runtime_error(sErrorMessage);
    }

    // Parse the response
    CKeePassResponse response(sKey, sResponse);

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

void CKeePassIntegrator::rpcAssociate(std::string& sId, SecureString& sKeyBase64)
{
    sKey = generateRandomKey(KEEPASS_CRYPTO_KEY_SIZE);
    CKeePassRequest request(sKey, "associate");

    sKeyBase64 = EncodeBase64Secure(sKey);
    request.addStrParameter("Key", std::string(&sKeyBase64[0], sKeyBase64.size()));

    int nStatus;
    std::string sResponse;

    doHTTPPost(request.getJson(), nStatus, sResponse);

    LogPrint("keepass", "CKeePassIntegrator::rpcAssociate -- send result: status: %d response: %s\n", nStatus, sResponse);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned: HTTP code ";
        sErrorMessage += itostr(nStatus);
        sErrorMessage += " - Response: ";
        sErrorMessage += " response: [";
        sErrorMessage += sResponse;
        sErrorMessage += "]";
        throw std::runtime_error(sErrorMessage);
    }

    // Parse the response
    CKeePassResponse response(sKey, sResponse);

    if(!response.getSuccess())
    {
        throw std::runtime_error("KeePassHttp returned failure status");
    }

    // If we got here, we were successful. Return the information
    sId = response.getStr("Id");
}

// Retrieve wallet passphrase from KeePass
SecureString CKeePassIntegrator::retrievePassphrase()
{

    // Check we have all required information
    if(sKey.size() == 0)
    {
        throw std::runtime_error("keepasskey parameter is not defined. Please specify the configuration parameter.");
    }
    if(sKeePassId.size() == 0)
    {
        throw std::runtime_error("keepassid parameter is not defined. Please specify the configuration parameter.");
    }
    if(sKeePassEntryName == "")
    {
        throw std::runtime_error("keepassname parameter is not defined. Please specify the configuration parameter.");
    }

    // Retrieve matching logins from KeePass
    std::vector<CKeePassIntegrator::CKeePassEntry>  entries = rpcGetLogins();

    // Only accept one unique match
    if(entries.size() == 0)
    {
        throw std::runtime_error("KeePassHttp returned 0 matches, please verify the keepassurl setting.");
    }
    if(entries.size() > 1)
    {
        throw std::runtime_error("KeePassHttp returned multiple matches, bailing out.");
    }

    return entries[0].getPassword();
}

// Update wallet passphrase in keepass
void CKeePassIntegrator::updatePassphrase(const SecureString& sWalletPassphrase)
{
    // Check we have all required information
    if(sKey.size() == 0)
    {
        throw std::runtime_error("keepasskey parameter is not defined. Please specify the configuration parameter.");
    }
    if(sKeePassId.size() == 0)
    {
        throw std::runtime_error("keepassid parameter is not defined. Please specify the configuration parameter.");
    }
    if(sKeePassEntryName == "")
    {
        throw std::runtime_error("keepassname parameter is not defined. Please specify the configuration parameter.");
    }

    SecureString sEntryId("");

    std::string sErrorMessage;

    // Lookup existing entry
    std::vector<CKeePassIntegrator::CKeePassEntry> vEntries = rpcGetLogins();

    if(vEntries.size() > 1)
    {
        throw std::runtime_error("KeePassHttp returned multiple matches, bailing out.");
    }

    if(vEntries.size() == 1)
    {
        sEntryId = vEntries[0].getUuid();
    }

    // Update wallet passphrase in KeePass
    rpcSetLogin(sWalletPassphrase, sEntryId);
}
