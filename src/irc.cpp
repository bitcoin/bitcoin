// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "irc.h"
#include "base58.h"
#include "net.h"

using namespace std;

int nGotIRCAddresses = 0;

void ThreadIRCSeed2(void* parg);




#pragma pack(push, 1)
struct ircaddr
{
    struct in_addr ip;
    unsigned short port;
};
#pragma pack(pop)

string EncodeAddress(const CService& addr)
{
    struct ircaddr tmp;
    if (addr.GetInAddr(&tmp.ip))
    {
        tmp.port = htons(addr.GetPort());

        vector<unsigned char> vch(UBEGIN(tmp), UEND(tmp));
        return string("u") + EncodeBase58Check(vch);
    }
    return "";
}

bool DecodeAddress(string str, CService& addr)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(str.substr(1), vch))
        return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr = CService(tmp.ip, ntohs(tmp.port));
    return true;
}






static bool Send(SOCKET hSocket, const char* pszSend)
{
    if (strstr(pszSend, "PONG") != pszSend)
        printf("IRC SENDING: %s\n", pszSend);
    const char* psz = pszSend;
    const char* pszEnd = psz + strlen(psz);
    while (psz < pszEnd)
    {
        int ret = send(hSocket, psz, pszEnd - psz, MSG_NOSIGNAL);
        if (ret < 0)
            return false;
        psz += ret;
    }
    return true;
}

bool RecvLineIRC(SOCKET hSocket, string& strLine)
{
    for ( ; ; )
    {
        bool fRet = RecvLine(hSocket, strLine);
        if (fRet)
        {
            if (fShutdown)
                return false;
            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() >= 1 && vWords[0] == "PING")
            {
                strLine[1] = 'O';
                strLine += '\r';
                Send(hSocket, strLine.c_str());
                continue;
            }
        }
        return fRet;
    }
}

int RecvUntil(SOCKET hSocket, const char* psz1, const char* psz2=NULL, const char* psz3=NULL, const char* psz4=NULL)
{
    for ( ; ; )
    {
        string strLine;
        strLine.reserve(10000);
        if (!RecvLineIRC(hSocket, strLine))
            return 0;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != string::npos)
            return 1;
        if (psz2 && strLine.find(psz2) != string::npos)
            return 2;
        if (psz3 && strLine.find(psz3) != string::npos)
            return 3;
        if (psz4 && strLine.find(psz4) != string::npos)
            return 4;
    }
}

bool Wait(int nSeconds)
{
    if (fShutdown)
        return false;
    printf("IRC waiting %d seconds to reconnect\n", nSeconds);
    for (int i = 0; i < nSeconds; i++)
    {
        if (fShutdown)
            return false;
        Sleep(1000);
    }
    return true;
}

bool RecvCodeLine(SOCKET hSocket, const char* psz1, string& strRet)
{
    strRet.clear();
    for ( ; ; )
    {
        string strLine;
        if (!RecvLineIRC(hSocket, strLine))
            return false;

        vector<string> vWords;
        ParseString(strLine, ' ', vWords);
        if (vWords.size() < 2)
            continue;

        if (vWords[1] == psz1)
        {
            printf("IRC %s\n", strLine.c_str());
            strRet = strLine;
            return true;
        }
    }
}

bool GetIPFromIRC(SOCKET hSocket, string strMyName, CNetAddr& ipRet)
{
    Send(hSocket, strprintf("USERHOST %s\r", strMyName.c_str()).c_str());

    string strLine;
    if (!RecvCodeLine(hSocket, "302", strLine))
        return false;

    vector<string> vWords;
    ParseString(strLine, ' ', vWords);
    if (vWords.size() < 4)
        return false;

    string str = vWords[3];
    if (str.rfind("@") == string::npos)
        return false;
    string strHost = str.substr(str.rfind("@")+1);

    // Hybrid IRC used by lfnet always returns IP when you userhost yourself,
    // but in case another IRC is ever used this should work.
    printf("GetIPFromIRC() got userhost %s\n", strHost.c_str());
    CNetAddr addr(strHost, true);
    if (!addr.IsValid())
        return false;
    ipRet = addr;

    return true;
}



void ThreadIRCSeed(void* parg)
{
    // Make this thread recognisable as the IRC seeding thread
    RenameThread("novacoin-ircseed");

    printf("ThreadIRCSeed started\n");

    try
    {
        ThreadIRCSeed2(parg);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "ThreadIRCSeed()");
    } catch (...) {
        PrintExceptionContinue(NULL, "ThreadIRCSeed()");
    }
    printf("ThreadIRCSeed exited\n");
}

void ThreadIRCSeed2(void* parg)
{
    // Don't connect to IRC if we won't use IPv4 connections.
    if (IsLimited(NET_IPV4))
        return;

    // ... or if we won't make outbound connections and won't accept inbound ones.
    if (mapArgs.count("-connect") && fNoListen)
        return;

    // ... or if IRC is not enabled.
    if (!GetBoolArg("-irc", true))
        return;

    printf("ThreadIRCSeed trying to connect...\n");

    int nErrorWait = 10;
    int nRetryWait = 10;
    int nNameRetry = 0;

    while (!fShutdown)
    {
        const uint16_t nIrcPort = 6667;
        CService addrConnect("92.243.23.21", nIrcPort); // irc.lfnet.org

        CService addrIRC("irc.lfnet.org", nIrcPort, true);
        if (addrIRC.IsValid())
            addrConnect = addrIRC;

        SOCKET hSocket;
        if (!ConnectSocket(addrConnect, hSocket))
        {
            printf("IRC connect failed\n");
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }

        if (!RecvUntil(hSocket, "Found your hostname", "using your IP address instead", "Couldn't look up your hostname", "ignoring hostname"))
        {
            CloseSocket(hSocket);
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }

        CNetAddr addrIPv4("1.2.3.4"); // arbitrary IPv4 address to make GetLocal prefer IPv4 addresses
        CService addrLocal;
        string strMyName;
        // Don't use our IP as our nick if we're not listening
        // or if it keeps failing because the nick is already in use.
        if (!fNoListen && GetLocal(addrLocal, &addrIPv4) && nNameRetry<3)
            strMyName = EncodeAddress(GetLocalAddress(&addrConnect));
        if (strMyName.empty())
            strMyName = strprintf("x%" PRIu64 "", GetRand(1000000000));

        Send(hSocket, strprintf("NICK %s\r", strMyName.c_str()).c_str());
        Send(hSocket, strprintf("USER %s 8 * : %s\r", strMyName.c_str(), strMyName.c_str()).c_str());

        int nRet = RecvUntil(hSocket, " 004 ", " 433 ");
        if (nRet != 1)
        {
            CloseSocket(hSocket);
            if (nRet == 2)
            {
                printf("IRC name already in use\n");
                nNameRetry++;
                Wait(10);
                continue;
            }
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }
        nNameRetry = 0;
        Sleep(500);

        // Get our external IP from the IRC server and re-nick before joining the channel
        CNetAddr addrFromIRC;
        if (GetIPFromIRC(hSocket, strMyName, addrFromIRC))
        {
            printf("GetIPFromIRC() returned %s\n", addrFromIRC.ToString().c_str());
            // Don't use our IP as our nick if we're not listening
            if (!fNoListen && addrFromIRC.IsRoutable())
            {
                // IRC lets you to re-nick
                AddLocal(addrFromIRC, LOCAL_IRC);
                strMyName = EncodeAddress(GetLocalAddress(&addrConnect));
                Send(hSocket, strprintf("NICK %s\r", strMyName.c_str()).c_str());
            }
        }

        if (fTestNet) {
            Send(hSocket, "JOIN #novacoinTEST2\r");
            Send(hSocket, "WHO #novacoinTEST2\r");
        } else {
            // randomly join #novacoin00-#novacoin05
            // int channel_number = GetRandInt(5);

            // Channel number is always 0 for initial release
            int channel_number = 0;
            Send(hSocket, strprintf("JOIN #novacoin%02d\r", channel_number).c_str());
            Send(hSocket, strprintf("WHO #novacoin%02d\r", channel_number).c_str());
        }

        int64_t nStart = GetTime();
        string strLine;
        strLine.reserve(10000);
        while (!fShutdown && RecvLineIRC(hSocket, strLine))
        {
            if (strLine.empty() || strLine.size() > 900 || strLine[0] != ':')
                continue;

            vector<string> vWords;
            ParseString(strLine, ' ', vWords);
            if (vWords.size() < 2)
                continue;

            std::string strName;

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strName = vWords[7];
                printf("IRC got who\n");
            }

            if (vWords[1] == "JOIN" && vWords[0].size() > 1)
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strName = vWords[0].substr(1, vWords[0].find('!', 1) - 1);
                printf("IRC got join\n");
            }

            if (strName.compare(0,1, "u") == 0)
            {
                CAddress addr;
                if (DecodeAddress(strName, addr))
                {
                    addr.nTime = GetAdjustedTime();
                    if (addrman.Add(addr, addrConnect, 51 * 60))
                        printf("IRC got new address: %s\n", addr.ToString().c_str());
                    nGotIRCAddresses++;
                }
                else
                {
                    printf("IRC decode failed\n");
                }
            }
        }
        CloseSocket(hSocket);

        if (GetTime() - nStart > 20 * 60)
        {
            nErrorWait /= 3;
            nRetryWait /= 3;
        }

        nRetryWait = nRetryWait * 11 / 10;
        if (!Wait(nRetryWait += 60))
            return;
    }
}










#ifdef TEST
int main(int argc, char *argv[])
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return false;
    }

    ThreadIRCSeed(NULL);

    WSACleanup();
    return 0;
}
#endif
