#include "resolv.h"

#include <boost/lexical_cast.hpp>

// callback used to write response from the server
static int writer(char *pData, size_t nSize, size_t nNmemb, std::string *pBuffer)  
{  
  int nResult = 0;  
  if (pBuffer != NULL)  
  {  
    pBuffer->append(pData, nSize * nNmemb);  
    // How much did we write?  
    nResult = nSize * nNmemb;  
  }  
  return nResult;  
}

NameResolutionService::NameResolutionService()
{
    // Initialise CURL with our various options.
    curl = curl_easy_init();
    // This goes first in case of any problems below. We get an error message.
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, pErrorBuffer);  
    // fail when server sends >= 404
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);  
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);  
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);  
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);  
    // server response goes in strBuffer
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strBuffer);  
    pErrorBuffer[0] = '\0';
}
NameResolutionService::~NameResolutionService()
{
    curl_easy_cleanup(curl);
}

void NameResolutionService::ExplodeHandle(const string& strHandle, string& strNickname, string& strDomain)
{
    // split address at @ furthrest to the right
    size_t nPosAtsym = strHandle.rfind('@');
    strNickname = strHandle.substr(0, nPosAtsym);
    strDomain = strHandle.substr(nPosAtsym + 1, strHandle.size());
}
bool NameResolutionService::Perform()
{
    // Called after everything has been setup. This actually does the request.
    CURLcode result = curl_easy_perform(curl);  
    return (result == CURLE_OK);
}

string NameResolutionService::FetchAddress(const string& strHandle, string& strAddy)
{
    // GET is defined for 'getting' data, so we use GET for the low risk fetching of people's addresses
    if (!curl)
        // For some reason CURL didn't start...
        return pErrorBuffer;
    // Expand the handle
    string strNickname, strDomain;
    ExplodeHandle(strHandle, strNickname, strDomain);
    // url encode the nickname for get request
    const char* pszEncodedNick = curl_easy_escape(curl, strNickname.c_str(), strNickname.size());
    if (!pszEncodedNick)
        return "Unable to encode nickname.";
    // construct url for GET request
    string strRequestUrl = strDomain + "/getaddress/?nickname=" + pszEncodedNick;
    // Pass URL to CURL
    curl_easy_setopt(curl, CURLOPT_URL, strRequestUrl.c_str());  
    if (!Perform())
        return pErrorBuffer;
    // Server should respond with a JSON that has the address in.
    strAddy = strBuffer;
    return "";  // no error
}

NameResolutionService::PostVariables::PostVariables()
{
    // pBegin/pEnd *must* be null before calling curl_formadd
    pBegin = NULL;
    pEnd = NULL;
}
NameResolutionService::PostVariables::~PostVariables()
{
    curl_formfree(pBegin);
}
bool NameResolutionService::PostVariables::Add(const string& strKey, const string& strVal)
{
    // Copy strings to this block. Return true on success.
    return curl_formadd(&pBegin, &pEnd, CURLFORM_COPYNAME, strKey.c_str(), CURLFORM_COPYCONTENTS, strVal.c_str(), CURLFORM_END) == CURL_FORMADD_OK;
}

curl_httppost* NameResolutionService::PostVariables::operator()() const
{
    return pBegin;
}

