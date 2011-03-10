#include "resolv.h"

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
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, pErrorBuffer);  
    // fail when server sends >= 404
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);  
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);  
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);  
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strBuffer);  
    pErrorBuffer[0] = '\0';
}
NameResolutionService::~NameResolutionService()
{
    curl_easy_cleanup(curl);
}

void NameResolutionService::ExplodeRef(const string& strRef, string& strNickname, string& strDomain)
{
    // split address at @ furthrest to the right
    size_t nPosAtsym = strRef.rfind('@');
    strNickname = strRef.substr(0, nPosAtsym);
    strDomain = strRef.substr(nPosAtsym + 1, strRef.size());
}
bool NameResolutionService::Perform()
{
    CURLcode result = curl_easy_perform(curl);  
    return (result != CURLE_OK);
}

#include <iostream>
string NameResolutionService::FetchAddress(const string& strRef, string& strAddy)
{
    if (!curl)
        return pErrorBuffer;
    string strNickname, strDomain;
    ExplodeRef(strRef, strNickname, strDomain);
    // url encode the nickname for get request
    const char* pszEncodedNick = curl_easy_escape(curl, strNickname.c_str(), strNickname.size());
    if (!pszEncodedNick)
        return "Unable to encode nickname.";
    // construct url for GET request
    string strRequestUrl = strDomain + "/getaddress.php?nickname=" + pszEncodedNick;
    curl_easy_setopt(curl, CURLOPT_URL, strRequestUrl.c_str());  
    if (Perform()) {
        return pErrorBuffer;
    }
    strAddy = strBuffer;
    return "";  // no error
}
string NameResolutionService::PushAddress(const string& strRef, const string& strPassword, const string& strNewaddy, string& strStatus)
{
    if (!curl)
        return pErrorBuffer;
    string strNickname, strDomain;
    ExplodeRef(strRef, strNickname, strDomain);
    std::cout << "PushAddress(" << strNickname << "," << strDomain << "," << strPassword << "," << strNewaddy << ")\n";
    return "";  // no error
}
string NameResolutionService::ChangePassword(const string& strRef, const string& strPassword, const string& strNewPassword, string& strStatus)
{
    if (!curl)
        return pErrorBuffer;
    string strNickname, strDomain;
    ExplodeRef(strRef, strNickname, strDomain);
    return "";  // no error
}

