// Copyright (c) 2014 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keepass.h"

#include <exception>
// #include <openssl/rand.h>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
// #include <boost/asio.hpp>

#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_reader_template.h"

#include "crypter.h"
#include "clientversion.h"
#include "random.h"
#include "rpcprotocol.h"

// Necessary to prevent compile errors due to forward declaration of
//CScript in serialize.h (included from crypter.h)
#include "script/script.h"
#include "script/standard.h"

#include "util.h"
#include "utilstrencodings.h"

using boost::asio::ip::tcp;

CKeePassIntegrator keePassInt;

CKeePassIntegrator::CKeePassIntegrator()
    :sKeyBase64(" "), sKey(" "), sUrl(" ") // Prevent LockedPageManagerBase complaints
{
    sKeyBase64.clear(); // Prevent LockedPageManagerBase complaints
    sKey.clear(); // Prevent LockedPageManagerBase complaints
    sUrl.clear(); // Prevent LockedPageManagerBase complaints
    bIsActive = false;
    nPort = KEEPASS_KEEPASSHTTP_PORT;
}

// Initialze from application context
void CKeePassIntegrator::init()
{
    bIsActive = GetBoolArg("-keepass", false);
    nPort = boost::lexical_cast<unsigned int>(GetArg("-keepassport", KEEPASS_KEEPASSHTTP_PORT));
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
    requestObj.push_back(json_spirit::Pair(sName, sValue));
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
    return json_spirit::write_string(json_spirit::Value(requestObj), false);
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
    json_spirit::Value responseValue;
    if(!json_spirit::read_string(sResponse, responseValue))
    {
        throw std::runtime_error("Unable to parse KeePassHttp response");
    }

    responseObj = responseValue.get_obj();

    // retrieve main values
    bSuccess = json_spirit::find_value(responseObj, "Success").get_bool();
    sType = getStr("RequestType");
    sIV = DecodeBase64(getStr("Nonce"));
}

std::string CKeePassIntegrator::CKeePassResponse::getStr(std::string sName)
{
    std::string sValue(json_spirit::find_value(responseObj, sName).get_str());
    return sValue;
}

SecureString CKeePassIntegrator::CKeePassResponse::getSecureStr(std::string sName)
{
    std::string sValueBase64Encrypted(json_spirit::find_value(responseObj, sName).get_str());
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

    json_spirit::Array aEntries = json_spirit::find_value(responseObj, "Entries").get_array();
    for(json_spirit::Array::iterator it = aEntries.begin(); it != aEntries.end(); ++it)
    {
        SecureString sEntryUuid(decrypt(json_spirit::find_value((*it).get_obj(), "Uuid").get_str().c_str()));
        SecureString sEntryName(decrypt(json_spirit::find_value((*it).get_obj(), "Name").get_str().c_str()));
        SecureString sEntryLogin(decrypt(json_spirit::find_value((*it).get_obj(), "Login").get_str().c_str()));
        SecureString sEntryPassword(decrypt(json_spirit::find_value((*it).get_obj(), "Password").get_str().c_str()));
        CKeePassEntry entry(sEntryUuid, sEntryUuid, sEntryLogin, sEntryPassword);
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
    RAND_bytes((unsigned char *) &key[0], nSize);

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

// Send RPC message to KeePassHttp
void CKeePassIntegrator::doHTTPPost(const std::string& sRequest, int& nStatus, std::string& sResponse)
{
    // Prepare communication
    boost::asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(KEEPASS_KEEPASSHTTP_HOST, boost::lexical_cast<std::string>(nPort));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }

    if(error)
    {
        throw boost::system::system_error(error);
    }

    // Form the request.
    std::map<std::string, std::string> mapRequestHeaders;
    std::string strPost = constructHTTPPost(sRequest, mapRequestHeaders);

    // Logging of actual post data disabled as to not write passphrase in debug.log. Only enable temporarily when needed
    //LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - send POST data: %s\n", strPost);
    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - send POST data\n");

    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << strPost;

    // Send the request.
    boost::asio::write(socket, request);

    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - request written\n");

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - request status line read\n");

    // Receive HTTP reply status
    int nProto = 0;
    std::istream response_stream(&response);
    nStatus = ReadHTTPStatus(response_stream, nProto);

    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - reading response body start\n");
    // Read until EOF, writing data to output as we go.
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
    {
        if (error != boost::asio::error::eof)
        {
            if (error != 0)
            { // 0 is success
                throw boost::system::system_error(error);
            }
        }
    }
    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - reading response body end\n");

    // Receive HTTP reply message headers and body
    std::map<std::string, std::string> mapHeaders;
    ReadHTTPMessage(response_stream, mapHeaders, sResponse, nProto, std::numeric_limits<size_t>::max());
    LogPrint("keepass", "CKeePassIntegrator::doHTTPPost - Processed body\n");
}

void CKeePassIntegrator::rpcTestAssociation(bool bTriggerUnlock)
{
    CKeePassRequest request(sKey, "test-associate");
    request.addStrParameter("TriggerUnlock", std::string(bTriggerUnlock ? "true" : "false"));

    int nStatus;
    std::string sResponse;

    doHTTPPost(request.getJson(), nStatus, sResponse);

    LogPrint("keepass", "CKeePassIntegrator::rpcTestAssociation - send result: status: %d response: %s\n", nStatus, sResponse);
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
    //LogPrint("keepass", "CKeePassIntegrator::rpcGetLogins - send result: status: %d response: %s\n", nStatus, sResponse);
    LogPrint("keepass", "CKeePassIntegrator::rpcGetLogins - send result: status: %d\n", nStatus);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned by KeePassHttp: HTTP code ";
        sErrorMessage += boost::lexical_cast<std::string>(nStatus);
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

    LogPrint("keepass", "CKeePassIntegrator::rpcSetLogin - send Url: %s\n", sUrl);

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


    LogPrint("keepass", "CKeePassIntegrator::rpcSetLogin - send result: status: %d response: %s\n", nStatus, sResponse);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned: HTTP code ";
        sErrorMessage += boost::lexical_cast<std::string>(nStatus);
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

    LogPrint("keepass", "CKeePassIntegrator::rpcAssociate - send result: status: %d response: %s\n", nStatus, sResponse);

    if(nStatus != 200)
    {
        std::string sErrorMessage = "Error returned: HTTP code ";
        sErrorMessage += boost::lexical_cast<std::string>(nStatus);
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
