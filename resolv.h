#ifndef NOMRESOLV_H__
#define NOMRESOLV_H__

#include <string>
#include "curl/curl.h"

using std::string;

/*

This class resolves against a server to lookup addresses.
To not conflict with the bitcoin addresses, we refer here to people's handles.
A handle is of the form:

   genjix@foo.org

Most characters are valid for the username + password (and handled accordingly), but the domain follows usual web standards. It is possible to affix a path if needed,

   genjix@bar.com/path/to/

*/

class NameResolutionService
{
public:
    NameResolutionService();
    ~NameResolutionService();

    // Three main methods map to RPC actions.
    string FetchAddress(const string& strHandle, string& strAddy);
    // Push new address to the server
    string UpdateAddress(const string& strHandle, const string& strNewaddy, string& strStatus);

private:
    // A POST block
    class PostVariables
    {
    public:
        PostVariables();
        ~PostVariables();
        // Add a new key, value pair
        bool Add(const string& strKey, const string& strVal);
        curl_httppost* operator()() const;
    private:
        // CURL stores POST blocks as linked lists.
        curl_httppost *pBegin, *pEnd;
    };

    // Explodes user@domain => user, domain
    static void ExplodeHandle(const string& strHandle, string& strNickname, string& strDomain);
    // Perform the HTTP request. Returns true on success.
    bool Perform();
    // Make a POST request that will modify the specified variable (strReqname) on the server.
    string MakeRequest(const string& strHandle, const string& strReqname, const string& strArgument, string& strStatus);

    // CURL error message
    char pErrorBuffer[CURL_ERROR_SIZE];  
    // CURL response
    string strBuffer;
    // CURL handle
    CURL *curl;
};

#endif

