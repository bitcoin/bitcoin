// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <update.h>

UpdateManager::UpdateManager()
{
    // Add sources from where to fetch the current version
    addSource("https://www.emilengler.com/bitcoin/version.txt");
    // Add keys to verify the current version (needs to be legacy)
    addKey("14RdN6xBjrrqx5y4nH7VXYgtyRVRvBQeF1"); // Emil Engler (@emilengler)
}

void UpdateManager::addSource(std::string url)
{
    sources.push_back(url);
}

void UpdateManager::addKey(std::string key)
{
    keys.push_back(key);
}

// Function to check if an item is in a string vector
bool VectorContains(std::vector<std::string> &v, std::string &s) {
    if (std::find(v.begin(), v.end(), s) != v.end()) {
        // Item is in vector
        return true;
    }
    return false;
}

// Checks for a new version and returns the newest version. If no new version is available or the source is not reachable it will return "false"
std::string UpdateManager::checkUpdate()
{
    // Ignore if the build isn't a release (needs to be activated before merge)
    /*if(!CLIENT_VERSION_IS_RELEASE)
        return "false";*/

    // For security use a pseudo random source
    srand(time(NULL));
    std::string source = sources[rand() % sources.size()];
    std::string upstreamVersion = download(source);

    // Repeat until a valid source was fetched
    std::vector<std::string> tried;                         // Sources that were already tried
    while (upstreamVersion == "ERROR") {
        tried.push_back(source);
        // Return "false" if no source is reachable
        if (tried.size() == sources.size())
            return "false";
        while (VectorContains(tried, source)) {
            source = sources[rand() % sources.size()];
        }
        upstreamVersion = download(source);
    }

    // Split upstreamVersion into "version" and "signature"
    std::vector<std::string> versionDetails;
    std::string line;
    std::istringstream lineStream(upstreamVersion);
    while (std::getline(lineStream, line, '\n')) {
        versionDetails.push_back(line);
    }

    // Verify authentity
    if (versionDetails.size() < 2)
        return "false";
    // Verify signature
    if (!verify(versionDetails[1], versionDetails[0])) {
        return "false";
    }

    // Check if the current version is the upstream version
    if (versionDetails[0] == FormatFullVersion())
        return "false";

    // Check for invalid characters (only dots and numbers are permitted)
    std::string legalChars = "v.0123456789";
    for(long unsigned int i = 0; i < versionDetails[0].size(); ++i) {
        if (legalChars.find(versionDetails[0][i]) == std::string::npos)
            return "false";
    }

    // Check if the version is newer (probably useless because of line 76)
    /*std::string currentVersion = FormatFullVersion();
    std::regex r("v0\.(.+?)$");
    std::smatch matches;
    while(std::regex_search(currentVersion, matches, r)) {
        // IF the current version is the same or higher, false
        if (atof(matches[1].str().c_str()) < atof(currentVersion.c_str()))
            return "false";
    }*/

    // If no errors, return the newest version
    return versionDetails[0];
}

// Just an internal function for making downloads possible
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::string data((const char*) ptr, (size_t) size * nmemb);
    *((std::stringstream*) stream) << data;
    return size * nmemb;
}


std::string UpdateManager::download(std::string url)
{
    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    std::stringstream out;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        return "ERROR";

    curl_easy_cleanup(curl);
    return out.str();
}

bool UpdateManager::verify(std::string strSign, std::string strMessage)
{
    for(unsigned long int i = 0; i < keys.size(); ++i)
    {
        std::string strAddress = keys[i];

        CTxDestination destination = DecodeDestination(strAddress);
        if (!IsValidDestination(destination)) {
            continue;
        }

        const PKHash *pkhash = boost::get<PKHash>(&destination);
        if (!pkhash) {
            continue;
        }

        bool fInvalid = false;
        std::vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

        if (fInvalid)
            continue;

        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << strMessage;

        CPubKey pubkey;
        if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
            continue;

        if (pubkey.GetID() == *pkhash)
            return true;
    }
    return false;
}