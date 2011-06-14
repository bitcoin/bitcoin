// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "irc.h"
#include "net.h"
#include "strlcpy.h"

using namespace std;
using namespace boost;

int nGotIRCAddresses = 0;
bool fGotExternalIP = false;

void ThreadIRCSeed2(void* parg);




#pragma pack(push, 1)
struct ircaddr
{
    int ip;
    short port;
};
#pragma pack(pop)

string EncodeAddress(const CAddress& addr)
{
    struct ircaddr tmp;
    tmp.ip    = addr.ip;
    tmp.port  = addr.port;

    vector<unsigned char> vch(UBEGIN(tmp), UEND(tmp));
    return string("u") + EncodeBase58Check(vch);
}

bool DecodeAddress(string str, CAddress& addr)
{
    vector<unsigned char> vch;
    if (!DecodeBase58Check(str.substr(1), vch))
        return false;

    struct ircaddr tmp;
    if (vch.size() != sizeof(tmp))
        return false;
    memcpy(&tmp, &vch[0], sizeof(tmp));

    addr = CAddress(tmp.ip, ntohs(tmp.port), NODE_NETWORK);
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

bool RecvLine(SOCKET hSocket, string& strLine)
{
    strLine = "";
    loop
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
            if (strLine.size() >= 9000)
                return true;
        }
        else if (nBytes <= 0)
        {
            if (fShutdown)
                return false;
            if (nBytes < 0)
            {
                int nErr = WSAGetLastError();
                if (nErr == WSAEMSGSIZE)
                    continue;
                if (nErr == WSAEWOULDBLOCK || nErr == WSAEINTR || nErr == WSAEINPROGRESS)
                {
                    Sleep(10);
                    continue;
                }
            }
            if (!strLine.empty())
                return true;
            if (nBytes == 0)
            {
                // socket closed
                printf("IRC socket closed\n");
                return false;
            }
            else
            {
                // socket error
                int nErr = WSAGetLastError();
                printf("IRC recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

bool RecvLineIRC(SOCKET hSocket, string& strLine)
{
    loop
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
    loop
    {
        string strLine;
        strLine.reserve(10000);
        if (!RecvLineIRC(hSocket, strLine))
            return 0;
        printf("IRC %s\n", strLine.c_str());
        if (psz1 && strLine.find(psz1) != -1)
            return 1;
        if (psz2 && strLine.find(psz2) != -1)
            return 2;
        if (psz3 && strLine.find(psz3) != -1)
            return 3;
        if (psz4 && strLine.find(psz4) != -1)
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
    loop
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

bool GetIPFromIRC(SOCKET hSocket, string strMyName, unsigned int& ipRet)
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
    if (fUseProxy)
        return false;
    CAddress addr(strHost, 0, true);
    if (!addr.IsValid())
        return false;
    ipRet = addr.ip;

    return true;
}



void ThreadIRCSeed(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadIRCSeed(parg));
    try
    {
        ThreadIRCSeed2(parg);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "ThreadIRCSeed()");
    } catch (...) {
        PrintExceptionContinue(NULL, "ThreadIRCSeed()");
    }
    printf("ThreadIRCSeed exiting\n");
}

void ThreadIRCSeed2(void* parg)
{
    /* Dont advertise on IRC if we don't allow incoming connections */
    if (mapArgs.count("-connect") || fNoListen)
        return;

    if (GetBoolArg("-noirc"))
        return;
    printf("ThreadIRCSeed started\n");
    int nErrorWait = 10;
    int nRetryWait = 10;
    bool fNameInUse = false;
    bool fTOR = (fUseProxy && addrProxy.port == htons(9050));

    while (!fShutdown)
    {
        //CAddress addrConnect("216.155.130.130:6667"); // chat.freenode.net
        CAddress addrConnect("92.243.23.21", 6667); // irc.lfnet.org
        if (!fTOR)
        {
            //struct hostent* phostent = gethostbyname("chat.freenode.net");
            CAddress addrIRC("irc.lfnet.org", 6667, true);
            if (addrIRC.IsValid())
                addrConnect = addrIRC;
        }

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
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }

        string strMyName;
        if (addrLocalHost.IsRoutable() && !fUseProxy && !fNameInUse)
            strMyName = EncodeAddress(addrLocalHost);
        else
            strMyName = strprintf("x%u", GetRand(1000000000));

        Send(hSocket, strprintf("NICK %s\r", strMyName.c_str()).c_str());
        Send(hSocket, strprintf("USER %s 8 * : %s\r", strMyName.c_str(), strMyName.c_str()).c_str());

        int nRet = RecvUntil(hSocket, " 004 ", " 433 ");
        if (nRet != 1)
        {
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
            if (nRet == 2)
            {
                printf("IRC name already in use\n");
                fNameInUse = true;
                Wait(10);
                continue;
            }
            nErrorWait = nErrorWait * 11 / 10;
            if (Wait(nErrorWait += 60))
                continue;
            else
                return;
        }
        Sleep(500);

        // Get our external IP from the IRC server and re-nick before joining the channel
        CAddress addrFromIRC;
        if (GetIPFromIRC(hSocket, strMyName, addrFromIRC.ip))
        {
            printf("GetIPFromIRC() returned %s\n", addrFromIRC.ToStringIP().c_str());
            if (!fUseProxy && addrFromIRC.IsRoutable())
            {
                // IRC lets you to re-nick
                fGotExternalIP = true;
                addrLocalHost.ip = addrFromIRC.ip;
                strMyName = EncodeAddress(addrLocalHost);
                Send(hSocket, strprintf("NICK %s\r", strMyName.c_str()).c_str());
            }
        }
        
        if (fTestNet) {
            Send(hSocket, "JOIN #bitcoinTEST\r");
            Send(hSocket, "WHO #bitcoinTEST\r");
        } else {
            // randomly join #bitcoin00-#bitcoin99
            int channel_number = GetRandInt(100);
            Send(hSocket, strprintf("JOIN #bitcoin%02d\r", channel_number).c_str());
            Send(hSocket, strprintf("WHO #bitcoin%02d\r", channel_number).c_str());
        }

        int64 nStart = GetTime();
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

            char pszName[10000];
            pszName[0] = '\0';

            if (vWords[1] == "352" && vWords.size() >= 8)
            {
                // index 7 is limited to 16 characters
                // could get full length name at index 10, but would be different from join messages
                strlcpy(pszName, vWords[7].c_str(), sizeof(pszName));
                printf("IRC got who\n");
            }

            if (vWords[1] == "JOIN" && vWords[0].size() > 1)
            {
                // :username!username@50000007.F000000B.90000002.IP JOIN :#channelname
                strlcpy(pszName, vWords[0].c_str() + 1, sizeof(pszName));
                if (strchr(pszName, '!'))
                    *strchr(pszName, '!') = '\0';
                printf("IRC got join\n");
            }

            if (pszName[0] == 'u')
            {
                CAddress addr;
                if (DecodeAddress(pszName, addr))
                {
                    addr.nTime = GetAdjustedTime();
                    if (AddAddress(addr, 51 * 60))
                        printf("IRC got new address: %s\n", addr.ToString().c_str());
                    nGotIRCAddresses++;
                }
                else
                {
                    printf("IRC decode failed\n");
                }
            }
        }
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;

        // IRC usually blocks TOR, so only try once
        if (fTOR)
            return;

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
