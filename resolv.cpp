#include "resolv.h"

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

string NameResolutionService::MakeRequest(const string& strHandle, const string& strPassword, const string& strReqname, const string& strArgument, string& strStatus)
{
    if (!curl)
        // For some reason CURL didn't start...
        return pErrorBuffer;
    // Expand the handle
    string strNickname, strDomain;
    ExplodeHandle(strHandle, strNickname, strDomain);
    // Construct URL for POST request
    string strRequestUrl = strDomain + "/set" + strReqname + "/";
    // Pass URL to CURL
    curl_easy_setopt(curl, CURLOPT_URL, strRequestUrl.c_str());  
    // Create our variables for POST
    PostVariables PostRequest;
    // This method is used by:
    //    setaddress  which takes a value called address
    //    setnewpassword which takes a value called newpassword
    if (!PostRequest.Add("nickname", strNickname) ||
        !PostRequest.Add("password", strPassword) ||
        !PostRequest.Add(strReqname, strArgument))
        return "Internal error constructing POST request.";
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, PostRequest()); 
    // Carry out request.
    if (!Perform())
        return pErrorBuffer;
    strStatus = strBuffer;
    return "";  // no error
}
string NameResolutionService::UpdateAddress(const string& strHandle, const string& strPassword, const string& strNewaddy, string& strStatus)
{
    return MakeRequest(strHandle, strPassword, "address", strNewaddy, strStatus);
}
string NameResolutionService::ChangePassword(const string& strHandle, const string& strPassword, const string& strNewPassword, string& strStatus)
{
    return MakeRequest(strHandle, strPassword, "newpassword", strNewPassword, strStatus);
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

