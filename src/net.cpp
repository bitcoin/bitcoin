// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "irc.h"
#include "db.h"
#include "net.h"
#include "init.h"
#include "strlcpy.h"
#include "addrman.h"
#include "ui_interface.h"

#ifdef WIN32
#include <string.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

using namespace std;
using namespace boost;

static const int MAX_OUTBOUND_CONNECTIONS = 8;

void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);
void ThreadOpenAddedConnections2(void* parg);
#ifdef USE_UPNP
void ThreadMapPort2(void* parg);
#endif
void ThreadDNSAddressSeed2(void* parg);
bool OpenNetworkConnection(const CAddress& addrConnect, bool fUseGrant = true);



//
// Global state variables
//
bool fClient = false;
bool fAllowDNS = false;
static bool fUseUPnP = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(CService("0.0.0.0", 0), nLocalServices);
CAddress addrSeenByPeer(CService("0.0.0.0", 0), nLocalServices);
static CNode* pnodeLocalHost = NULL;
uint64 nLocalHostNonce = 0;
array<int, THREAD_MAX> vnThreadsRunning;
static SOCKET hListenSocket = INVALID_SOCKET;
CAddrMan addrman;

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
map<CInv, CDataStream> mapRelay;
deque<pair<int64, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
map<CInv, int64> mapAlreadyAskedFor;


set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;

static CSemaphore *semOutbound = NULL;

unsigned short GetListenPort()
{
    return (unsigned short)(GetArg("-port", GetDefaultPort()));
}

void CNode::PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pindexLastGetBlocksBegin && hashEnd == hashLastGetBlocksEnd)
        return;
    pindexLastGetBlocksBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    PushMessage("getblocks", CBlockLocator(pindexBegin), hashEnd);
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
                printf("socket closed\n");
                return false;
            }
            else
            {
                // socket error
                int nErr = WSAGetLastError();
                printf("recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}



bool GetMyExternalIP2(const CService& addrConnect, const char* pszGet, const char* pszKeyword, CNetAddr& ipRet)
{
    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return error("GetMyExternalIP() : connection to %s failed", addrConnect.ToString().c_str());

    send(hSocket, pszGet, strlen(pszGet), MSG_NOSIGNAL);

    string strLine;
    while (RecvLine(hSocket, strLine))
    {
        if (strLine.empty()) // HTTP response is separated from headers by blank line
        {
            loop
            {
                if (!RecvLine(hSocket, strLine))
                {
                    closesocket(hSocket);
                    return false;
                }
                if (pszKeyword == NULL)
                    break;
                if (strLine.find(pszKeyword) != string::npos)
                {
                    strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
                    break;
                }
            }
            closesocket(hSocket);
            if (strLine.find("<") != string::npos)
                strLine = strLine.substr(0, strLine.find("<"));
            strLine = strLine.substr(strspn(strLine.c_str(), " \t\n\r"));
            while (strLine.size() > 0 && isspace(strLine[strLine.size()-1]))
                strLine.resize(strLine.size()-1);
            CService addr(strLine,0,true);
            printf("GetMyExternalIP() received [%s] %s\n", strLine.c_str(), addr.ToString().c_str());
            if (!addr.IsValid() || !addr.IsRoutable())
                return false;
            ipRet.SetIP(addr);
            return true;
        }
    }
    closesocket(hSocket);
    return error("GetMyExternalIP() : connection closed");
}

// We now get our external IP from the IRC server first and only use this as a backup
bool GetMyExternalIP(CNetAddr& ipRet)
{
    CService addrConnect;
    const char* pszGet;
    const char* pszKeyword;

    if (fNoListen||fUseProxy)
        return false;

    for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
        // We should be phasing out our use of sites like these.  If we need
        // replacements, we should ask for volunteers to put this simple
        // php file on their webserver that prints the client IP:
        //  <?php echo $_SERVER["REMOTE_ADDR"]; ?>
        if (nHost == 1)
        {
            addrConnect = CService("91.198.22.70",80); // checkip.dyndns.org

            if (nLookup == 1)
            {
                CService addrIP("checkip.dyndns.org", 80, true);
                if (addrIP.IsValid())
                    addrConnect = addrIP;
            }

            pszGet = "GET / HTTP/1.1\r\n"
                     "Host: checkip.dyndns.org\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "Address:";
        }
        else if (nHost == 2)
        {
            addrConnect = CService("74.208.43.192", 80); // www.showmyip.com

            if (nLookup == 1)
            {
                CService addrIP("www.showmyip.com", 80, true);
                if (addrIP.IsValid())
                    addrConnect = addrIP;
            }

            pszGet = "GET /simple/ HTTP/1.1\r\n"
                     "Host: www.showmyip.com\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = NULL; // Returns just IP address
        }

        if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
            return true;
    }

    return false;
}

void ThreadGetMyExternalIP(void* parg)
{
    // Wait for IRC to get it first - disabled with ppcoin
    if (false && GetBoolArg("-irc", false))
    {
        for (int i = 0; i < 2 * 60; i++)
        {
            Sleep(1000);
            if (fGotExternalIP || fShutdown)
                return;
        }
    }

    // Fallback in case IRC fails to get it
    if (GetMyExternalIP(addrLocalHost))
    {
        printf("GetMyExternalIP() returned %s\n", addrLocalHost.ToStringIP().c_str());
        if (addrLocalHost.IsRoutable())
        {
            // If we already connected to a few before we had our IP, go back and addr them.
            // setAddrKnown automatically filters any duplicate sends.
            CAddress addr(addrLocalHost);
            addr.nTime = GetAdjustedTime();
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->PushAddress(addr);
            }
        }
    }
}





void AddressCurrentlyConnected(const CService& addr)
{
    addrman.Connected(addr);
}







CNode* FindNode(const CNetAddr& ip)
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CNetAddr)pnode->addr == ip)
                return (pnode);
    }
    return NULL;
}

CNode* FindNode(const CService& addr)
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CService)pnode->addr == addr)
                return (pnode);
    }
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, int64 nTimeout)
{
    if ((CNetAddr)addrConnect == (CNetAddr)addrLocalHost)
        return NULL;

    // Look for an existing connection
    CNode* pnode = FindNode((CService)addrConnect);
    if (pnode)
    {
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
        return pnode;
    }

    /// debug print
    printf("trying connection %s lastseen=%.1fhrs\n",
        addrConnect.ToString().c_str(),
        (double)(addrConnect.nTime - GetAdjustedTime())/3600.0);

    addrman.Attempt(addrConnect);

    // Connect
    SOCKET hSocket;
    if (ConnectSocket(addrConnect, hSocket))
    {
        /// debug print
        printf("connected %s\n", addrConnect.ToString().c_str());

        // Set to nonblocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            printf("ConnectSocket() : ioctlsocket nonblocking setting failed, error %d\n", WSAGetLastError());
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            printf("ConnectSocket() : fcntl nonblocking setting failed, error %d\n", errno);
#endif

        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, false);
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
        {
            LOCK(cs_vNodes);
            vNodes.push_back(pnode);
        }

        pnode->nTimeConnected = GetTime();
        return pnode;
    }
    else
    {
        return NULL;
    }
}

void CNode::CloseSocketDisconnect()
{
    fDisconnect = true;
    if (hSocket != INVALID_SOCKET)
    {
        if (fDebug)
            printf("%s ", DateTimeStrFormat(GetTime()).c_str());
        printf("disconnecting node %s\n", addr.ToString().c_str());
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
        vRecv.clear();
    }
}

void CNode::Cleanup()
{
}


void CNode::PushVersion()
{
    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (fUseProxy ? CAddress(CService("0.0.0.0",0)) : addr);
    CAddress addrMe = (fUseProxy || !addrLocalHost.IsRoutable() ? CAddress(CService("0.0.0.0",0)) : addrLocalHost);
    RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
    PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
                nLocalHostNonce, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<string>()), nBestHeight);
}





std::map<CNetAddr, int64> CNode::setBanned;
CCriticalSection CNode::cs_setBanned;

void CNode::ClearBanned()
{
    setBanned.clear();
}

bool CNode::IsBanned(CNetAddr ip)
{
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        std::map<CNetAddr, int64>::iterator i = setBanned.find(ip);
        if (i != setBanned.end())
        {
            int64 t = (*i).second;
            if (GetTime() < t)
                fResult = true;
        }
    }
    return fResult;
}

bool CNode::Misbehaving(int howmuch)
{
    if (addr.IsLocal())
    {
        printf("Warning: local node %s misbehaving\n", addr.ToString().c_str());
        return false;
    }

    nMisbehavior += howmuch;
    if (nMisbehavior >= GetArg("-banscore", 100))
    {
        int64 banTime = GetTime()+GetArg("-bantime", 60*60*24);  // Default 24-hour ban
        {
            LOCK(cs_setBanned);
            if (setBanned[addr] < banTime)
                setBanned[addr] = banTime;
        }
        CloseSocketDisconnect();
        printf("Disconnected %s for misbehavior (score=%d)\n", addr.ToString().c_str(), nMisbehavior);
        return true;
    }
    return false;
}












void ThreadSocketHandler(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadSocketHandler(parg));
    try
    {
        vnThreadsRunning[THREAD_SOCKETHANDLER]++;
        ThreadSocketHandler2(parg);
        vnThreadsRunning[THREAD_SOCKETHANDLER]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_SOCKETHANDLER]--;
        PrintException(&e, "ThreadSocketHandler()");
    } catch (...) {
        vnThreadsRunning[THREAD_SOCKETHANDLER]--;
        throw; // support pthread_cancel()
    }
    printf("ThreadSocketHandler exiting\n");
}

void ThreadSocketHandler2(void* parg)
{
    printf("ThreadSocketHandler started\n");
    list<CNode*> vNodesDisconnected;
    unsigned int nPrevNodeCount = 0;

    loop
    {
        //
        // Disconnect nodes
        //
        {
            LOCK(cs_vNodes);
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
            {
                if (pnode->fDisconnect ||
                    (pnode->GetRefCount() <= 0 && pnode->vRecv.empty() && pnode->vSend.empty()))
                {
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

                    if (pnode->fHasGrant)
                        semOutbound->post();
                    pnode->fHasGrant = false;

                    // close socket and cleanup
                    pnode->CloseSocketDisconnect();
                    pnode->Cleanup();

                    // hold in disconnected pool until all refs are released
                    pnode->nReleaseTime = max(pnode->nReleaseTime, GetTime() + 15 * 60);
                    if (pnode->fNetworkNode || pnode->fInbound)
                        pnode->Release();
                    vNodesDisconnected.push_back(pnode);
                }
            }

            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
            BOOST_FOREACH(CNode* pnode, vNodesDisconnectedCopy)
            {
                // wait until threads are done using it
                if (pnode->GetRefCount() <= 0)
                {
                    bool fDelete = false;
                    {
                        TRY_LOCK(pnode->cs_vSend, lockSend);
                        if (lockSend)
                        {
                            TRY_LOCK(pnode->cs_vRecv, lockRecv);
                            if (lockRecv)
                            {
                                TRY_LOCK(pnode->cs_mapRequests, lockReq);
                                if (lockReq)
                                {
                                    TRY_LOCK(pnode->cs_inventory, lockInv);
                                    if (lockInv)
                                        fDelete = true;
                                }
                            }
                        }
                    }
                    if (fDelete)
                    {
                        vNodesDisconnected.remove(pnode);
                        delete pnode;
                    }
                }
            }
        }
        if (vNodes.size() != nPrevNodeCount)
        {
            nPrevNodeCount = vNodes.size();
            MainFrameRepaint();
        }


        //
        // Find which sockets have data to receive
        //
        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000; // frequency to poll pnode->vSend

        fd_set fdsetRecv;
        fd_set fdsetSend;
        fd_set fdsetError;
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        FD_ZERO(&fdsetError);
        SOCKET hSocketMax = 0;

        if(hListenSocket != INVALID_SOCKET)
            FD_SET(hListenSocket, &fdsetRecv);
        hSocketMax = max(hSocketMax, hListenSocket);
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if (pnode->hSocket == INVALID_SOCKET)
                    continue;
                FD_SET(pnode->hSocket, &fdsetRecv);
                FD_SET(pnode->hSocket, &fdsetError);
                hSocketMax = max(hSocketMax, pnode->hSocket);
                {
                    TRY_LOCK(pnode->cs_vSend, lockSend);
                    if (lockSend && !pnode->vSend.empty())
                        FD_SET(pnode->hSocket, &fdsetSend);
                }
            }
        }

        vnThreadsRunning[THREAD_SOCKETHANDLER]--;
        int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
        vnThreadsRunning[THREAD_SOCKETHANDLER]++;
        if (fShutdown)
            return;
        if (nSelect == SOCKET_ERROR)
        {
            int nErr = WSAGetLastError();
            if (hSocketMax != INVALID_SOCKET)
            {
                printf("socket select error %d\n", nErr);
                for (unsigned int i = 0; i <= hSocketMax; i++)
                    FD_SET(i, &fdsetRecv);
            }
            FD_ZERO(&fdsetSend);
            FD_ZERO(&fdsetError);
            Sleep(timeout.tv_usec/1000);
        }


        //
        // Accept new connections
        //
        if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_in sockaddr;
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr;
            int nInbound = 0;

            if (hSocket != INVALID_SOCKET)
                addr = CAddress(sockaddr);

            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    if (pnode->fInbound)
                        nInbound++;
            }

            if (hSocket == INVALID_SOCKET)
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                    printf("socket error accept failed: %d\n", WSAGetLastError());
            }
            else if (nInbound >= GetArg("-maxconnections", 125) - MAX_OUTBOUND_CONNECTIONS)
            {
                {
                    LOCK(cs_setservAddNodeAddresses);
                    if (!setservAddNodeAddresses.count(addr))
                        closesocket(hSocket);
                }
            }
            else if (CNode::IsBanned(addr))
            {
                printf("connection from %s dropped (banned)\n", addr.ToString().c_str());
                closesocket(hSocket);
            }
            else
            {
                printf("accepted connection %s\n", addr.ToString().c_str());
                CNode* pnode = new CNode(hSocket, addr, true);
                pnode->AddRef();
                {
                    LOCK(cs_vNodes);
                    vNodes.push_back(pnode);
                }
            }
        }


        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            if (fShutdown)
                return;

            //
            // Receive
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetRecv) || FD_ISSET(pnode->hSocket, &fdsetError))
            {
                TRY_LOCK(pnode->cs_vRecv, lockRecv);
                if (lockRecv)
                {
                    CDataStream& vRecv = pnode->vRecv;
                    unsigned int nPos = vRecv.size();

                    if (nPos > ReceiveBufferSize()) {
                        if (!pnode->fDisconnect)
                            printf("socket recv flood control disconnect (%d bytes)\n", vRecv.size());
                        pnode->CloseSocketDisconnect();
                    }
                    else {
                        // typical socket buffer is 8K-64K
                        char pchBuf[0x10000];
                        int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            vRecv.resize(nPos + nBytes);
                            memcpy(&vRecv[nPos], pchBuf, nBytes);
                            pnode->nLastRecv = GetTime();
                        }
                        else if (nBytes == 0)
                        {
                            // socket closed gracefully
                            if (!pnode->fDisconnect)
                                printf("socket closed\n");
                            pnode->CloseSocketDisconnect();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                if (!pnode->fDisconnect)
                                    printf("socket recv error %d\n", nErr);
                                pnode->CloseSocketDisconnect();
                            }
                        }
                    }
                }
            }

            //
            // Send
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetSend))
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                {
                    CDataStream& vSend = pnode->vSend;
                    if (!vSend.empty())
                    {
                        int nBytes = send(pnode->hSocket, &vSend[0], vSend.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            vSend.erase(vSend.begin(), vSend.begin() + nBytes);
                            pnode->nLastSend = GetTime();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                printf("socket send error %d\n", nErr);
                                pnode->CloseSocketDisconnect();
                            }
                        }
                        if (vSend.size() > SendBufferSize()) {
                            if (!pnode->fDisconnect)
                                printf("socket send flood control disconnect (%d bytes)\n", vSend.size());
                            pnode->CloseSocketDisconnect();
                        }
                    }
                }
            }

            //
            // Inactivity checking
            //
            if (pnode->vSend.empty())
                pnode->nLastSendEmpty = GetTime();
            if (GetTime() - pnode->nTimeConnected > 60)
            {
                if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
                {
                    printf("socket no message in first 60 seconds, %d %d\n", pnode->nLastRecv != 0, pnode->nLastSend != 0);
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastSend > 90*60 && GetTime() - pnode->nLastSendEmpty > 90*60)
                {
                    printf("socket not sending\n");
                    pnode->fDisconnect = true;
                }
                else if (GetTime() - pnode->nLastRecv > 90*60)
                {
                    printf("socket inactivity timeout\n");
                    pnode->fDisconnect = true;
                }
            }
        }
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        Sleep(10);
    }
}









#ifdef USE_UPNP
void ThreadMapPort(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadMapPort(parg));
    try
    {
        vnThreadsRunning[THREAD_UPNP]++;
        ThreadMapPort2(parg);
        vnThreadsRunning[THREAD_UPNP]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_UPNP]--;
        PrintException(&e, "ThreadMapPort()");
    } catch (...) {
        vnThreadsRunning[THREAD_UPNP]--;
        PrintException(NULL, "ThreadMapPort()");
    }
    printf("ThreadMapPort exiting\n");
}

void ThreadMapPort2(void* parg)
{
    printf("ThreadMapPort started\n");

    char port[6];
    sprintf(port, "%d", GetListenPort());

    const char * multicastif = 0;
    const char * minissdpdpath = 0;
    struct UPNPDev * devlist = 0;
    char lanaddr[64];

#ifndef UPNPDISCOVER_SUCCESS
    /* miniupnpc 1.5 */
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
#else
    /* miniupnpc 1.6 */
    int error = 0;
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
#endif

    struct UPNPUrls urls;
    struct IGDdatas data;
    int r;

    r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (r == 1)
    {
        if (!addrLocalHost.IsRoutable())
        {
            char externalIPAddress[40];
            r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
            if(r != UPNPCOMMAND_SUCCESS)
                printf("UPnP: GetExternalIPAddress() returned %d\n", r);
            else
            {
                if(externalIPAddress[0])
                {
                    printf("UPnP: ExternalIPAddress = %s\n", externalIPAddress);
                    CAddress addrExternalFromUPnP(CService(externalIPAddress, 0), nLocalServices);
                    if (addrExternalFromUPnP.IsRoutable())
                        addrLocalHost = addrExternalFromUPnP;
                }
                else
                    printf("UPnP: GetExternalIPAddress failed.\n");
            }
        }

        string strDesc = "PPCoin " + FormatFullVersion();
#ifndef UPNPDISCOVER_SUCCESS
        /* miniupnpc 1.5 */
        r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            port, port, lanaddr, strDesc.c_str(), "TCP", 0);
#else
        /* miniupnpc 1.6 */
        r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
#endif

        if(r!=UPNPCOMMAND_SUCCESS)
            printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                port, port, lanaddr, r, strupnperror(r));
        else
            printf("UPnP Port Mapping successful.\n");
        int i = 1;
        loop {
            if (fShutdown || !fUseUPnP)
            {
                r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port, "TCP", 0);
                printf("UPNP_DeletePortMapping() returned : %d\n", r);
                freeUPNPDevlist(devlist); devlist = 0;
                FreeUPNPUrls(&urls);
                return;
            }
            if (i % 600 == 0) // Refresh every 20 minutes
            {
#ifndef UPNPDISCOVER_SUCCESS
                /* miniupnpc 1.5 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0);
#else
                /* miniupnpc 1.6 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
#endif

                if(r!=UPNPCOMMAND_SUCCESS)
                    printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
                        port, port, lanaddr, r, strupnperror(r));
                else
                    printf("UPnP Port Mapping successful.\n");;
            }
            Sleep(2000);
            i++;
        }
    } else {
        printf("No valid UPnP IGDs found\n");
        freeUPNPDevlist(devlist); devlist = 0;
        if (r != 0)
            FreeUPNPUrls(&urls);
        loop {
            if (fShutdown || !fUseUPnP)
                return;
            Sleep(2000);
        }
    }
}

void MapPort(bool fMapPort)
{
    if (fUseUPnP != fMapPort)
    {
        fUseUPnP = fMapPort;
    }
    if (fUseUPnP && vnThreadsRunning[THREAD_UPNP] < 1)
    {
        if (!CreateThread(ThreadMapPort, NULL))
            printf("Error: ThreadMapPort(ThreadMapPort) failed\n");
    }
}
#else
void MapPort(bool /* unused fMapPort */)
{
    // Intentionally left blank.
}
#endif









// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
// testnet dns seed begins with 't', all else are ppcoin dns seeds.
static const char *strDNSSeed[][2] = {
    {"seed", "seed.ppcoin.net"},
    {"tnseed", "tnseed.ppcoin.net"},
};

void ThreadDNSAddressSeed(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadDNSAddressSeed(parg));
    try
    {
        vnThreadsRunning[THREAD_DNSSEED]++;
        ThreadDNSAddressSeed2(parg);
        vnThreadsRunning[THREAD_DNSSEED]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_DNSSEED]--;
        PrintException(&e, "ThreadDNSAddressSeed()");
    } catch (...) {
        vnThreadsRunning[THREAD_DNSSEED]--;
        throw; // support pthread_cancel()
    }
    printf("ThreadDNSAddressSeed exiting\n");
}

void ThreadDNSAddressSeed2(void* parg)
{
    printf("ThreadDNSAddressSeed started\n");
    int found = 0;

    if (true /*!fTestNet*/)  // ppcoin enables dns seeding with testnet too
    {
        printf("Loading addresses from DNS seeds (could take a while)\n");

        for (unsigned int seed_idx = 0; seed_idx < ARRAYLEN(strDNSSeed); seed_idx++) {
            if (fTestNet && strDNSSeed[seed_idx][1][0] != 't') continue;
            if ((!fTestNet) && strDNSSeed[seed_idx][1][0] == 't') continue;

            vector<CNetAddr> vaddr;
            vector<CAddress> vAdd;
            if (LookupHost(strDNSSeed[seed_idx][1], vaddr))
            {
                BOOST_FOREACH(CNetAddr& ip, vaddr)
                {
                    int nOneDay = 24*3600;
                    CAddress addr = CAddress(CService(ip, GetDefaultPort()));
                    addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
            }
            addrman.Add(vAdd, CNetAddr(strDNSSeed[seed_idx][0], true));
        }
    }

    printf("%d addresses found from DNS seeds\n", found);
}












unsigned int pnSeed[] =
{
    0x231621b2,
};

void DumpAddresses()
{
    CAddrDB adb;
    adb.WriteAddrman(addrman);
}

void ThreadDumpAddress2(void* parg)
{
    vnThreadsRunning[THREAD_DUMPADDRESS]++;
    while (!fShutdown)
    {
        DumpAddresses();
        vnThreadsRunning[THREAD_DUMPADDRESS]--;
        Sleep(100000);
        vnThreadsRunning[THREAD_DUMPADDRESS]++;
    }
    vnThreadsRunning[THREAD_DUMPADDRESS]--;
}

void ThreadDumpAddress(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadDumpAddress(parg));
    try
    {
        ThreadDumpAddress2(parg);
    }
    catch (std::exception& e) {
        PrintException(&e, "ThreadDumpAddress()");
    }
    printf("ThreadDumpAddress exiting\n");
}

void ThreadOpenConnections(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadOpenConnections(parg));
    try
    {
        vnThreadsRunning[THREAD_OPENCONNECTIONS]++;
        ThreadOpenConnections2(parg);
        vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
        PrintException(&e, "ThreadOpenConnections()");
    } catch (...) {
        vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
        PrintException(NULL, "ThreadOpenConnections()");
    }
    printf("ThreadOpenConnections exiting\n");
}

void ThreadOpenConnections2(void* parg)
{
    printf("ThreadOpenConnections started\n");

    // Connect to specific addresses
    if (mapArgs.count("-connect"))
    {
        for (int64 nLoop = 0;; nLoop++)
        {
            BOOST_FOREACH(string strAddr, mapMultiArgs["-connect"])
            {
                CAddress addr(CService(strAddr, GetDefaultPort(), fAllowDNS));
                if (addr.IsValid())
                    OpenNetworkConnection(addr, false);
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    Sleep(500);
                    if (fShutdown)
                        return;
                }
            }
        }
    }

    // Initiate network connections
    int64 nStart = GetTime();
    loop
    {
        vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
        Sleep(500);
        vnThreadsRunning[THREAD_OPENCONNECTIONS]++;
        if (fShutdown)
            return;


        vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
        semOutbound->wait();
        vnThreadsRunning[THREAD_OPENCONNECTIONS]++;
        if (fShutdown)
            return;

        // Add seed nodes if IRC isn't working
        bool fTOR = (fUseProxy && addrProxy.GetPort() == 9050);
        if (addrman.size()==0 && (GetTime() - nStart > 60 || fTOR) && !fTestNet)
        {
            std::vector<CAddress> vAdd;
            for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
            {
                // It'll only connect to one or two seed nodes because once it connects,
                // it'll get a pile of addresses with newer timestamps.
                // Seed nodes are given a random 'last seen time' of between one and two
                // weeks ago.
                const int64 nOneWeek = 7*24*60*60;
                struct in_addr ip;
                memcpy(&ip, &pnSeed[i], sizeof(ip));
                CAddress addr(CService(ip, GetDefaultPort()));
                addr.nTime = GetTime()-GetRand(nOneWeek)-nOneWeek;
                vAdd.push_back(addr);
            }
            addrman.Add(vAdd, CNetAddr("127.0.0.1"));
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect to one address per a.b.?.? range.
        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
        int nOutbound = 0;
        set<vector<unsigned char> > setConnected;
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes) {
                setConnected.insert(pnode->addr.GetGroup());
                if (!pnode->fInbound)
                    nOutbound++;
            }
        }

        int64 nANow = GetAdjustedTime();

        int nTries = 0;
        loop
        {
            // use an nUnkBias between 10 (no outgoing connections) and 90 (8 outgoing connections)
            CAddress addr = addrman.Select(10 + min(nOutbound,8)*10);

            // if we selected an invalid address, restart
            if (!addr.IsIPv4() || !addr.IsValid() || setConnected.count(addr.GetGroup()) || addr == addrLocalHost)
                break;

            nTries++;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if (addr.GetPort() != GetDefaultPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect);
        else
            semOutbound->post();
    }
}

void ThreadOpenAddedConnections(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadOpenAddedConnections(parg));
    try
    {
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]++;
        ThreadOpenAddedConnections2(parg);
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
        PrintException(&e, "ThreadOpenAddedConnections()");
    } catch (...) {
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
        PrintException(NULL, "ThreadOpenAddedConnections()");
    }
    printf("ThreadOpenAddedConnections exiting\n");
}

void ThreadOpenAddedConnections2(void* parg)
{
    printf("ThreadOpenAddedConnections started\n");

    if (mapArgs.count("-addnode") == 0)
        return;

    vector<vector<CService> > vservAddressesToAdd(0);
    BOOST_FOREACH(string& strAddNode, mapMultiArgs["-addnode"])
    {
        vector<CService> vservNode(0);
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fAllowDNS, 0))
        {
            vservAddressesToAdd.push_back(vservNode);
            {
                LOCK(cs_setservAddNodeAddresses);
                BOOST_FOREACH(CService& serv, vservNode)
                    setservAddNodeAddresses.insert(serv);
            }
        }
    }
    loop
    {
        vector<vector<CService> > vservConnectAddresses = vservAddressesToAdd;
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fAllowDNS)
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                for (vector<vector<CService> >::iterator it = vservConnectAddresses.begin(); it != vservConnectAddresses.end(); it++)
                    BOOST_FOREACH(CService& addrNode, *(it))
                        if (pnode->addr == addrNode)
                        {
                            it = vservConnectAddresses.erase(it);
                            it--;
                            break;
                        }
        }
        BOOST_FOREACH(vector<CService>& vserv, vservConnectAddresses)
        {
            semOutbound->wait();
            OpenNetworkConnection(CAddress(*(vserv.begin())));
            Sleep(500);
            if (fShutdown)
                return;
        }
        if (fShutdown)
            return;
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
        Sleep(120000); // Retry every 2 minutes
        vnThreadsRunning[THREAD_ADDEDCONNECTIONS]++;
        if (fShutdown)
            return;
    }
}

bool static ReleaseGrant(bool fUseGrant) {
    if (fUseGrant)
        semOutbound->post();
    return false;
}

// only call this function when semOutbound has been waited for
bool OpenNetworkConnection(const CAddress& addrConnect, bool fUseGrant)
{
    //
    // Initiate outbound network connection
    //
    if (fShutdown)
        return false;
    if ((CNetAddr)addrConnect == (CNetAddr)addrLocalHost || !addrConnect.IsIPv4() ||
        FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect))
        return ReleaseGrant(fUseGrant);

    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    CNode* pnode = ConnectNode(addrConnect);
    vnThreadsRunning[THREAD_OPENCONNECTIONS]++;
    if (fShutdown)
        return false;
    if (!pnode)
        return ReleaseGrant(fUseGrant);
    if (pnode->fHasGrant) {
        // node already has connection grant, release the one that was passed to us
        ReleaseGrant(fUseGrant);
    } else {
        pnode->fHasGrant = fUseGrant;
    }
    pnode->fNetworkNode = true;

    return true;
}








void ThreadMessageHandler(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadMessageHandler(parg));
    try
    {
        vnThreadsRunning[THREAD_MESSAGEHANDLER]++;
        ThreadMessageHandler2(parg);
        vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
        PrintException(&e, "ThreadMessageHandler()");
    } catch (...) {
        vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
        PrintException(NULL, "ThreadMessageHandler()");
    }
    printf("ThreadMessageHandler exiting\n");
}

void ThreadMessageHandler2(void* parg)
{
    printf("ThreadMessageHandler started\n");
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (!fShutdown)
    {
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }

        // Poll the connected nodes for messages
        CNode* pnodeTrickle = NULL;
        if (!vNodesCopy.empty())
            pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            // Receive messages
            {
                TRY_LOCK(pnode->cs_vRecv, lockRecv);
                if (lockRecv)
                    ProcessMessages(pnode);
            }
            if (fShutdown)
                return;

            // Send messages
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                    SendMessages(pnode, pnode == pnodeTrickle);
            }
            if (fShutdown)
                return;
        }

        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        // Wait and allow messages to bunch up.
        // Reduce vnThreadsRunning so StopNode has permission to exit while
        // we're sleeping, but we must always check fShutdown after doing this.
        vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
        Sleep(100);
        if (fRequestShutdown)
            StartShutdown();
        vnThreadsRunning[THREAD_MESSAGEHANDLER]++;
        if (fShutdown)
            return;
    }
}

// ppcoin: stake minter thread
void static ThreadStakeMinter(void* parg)
{
    printf("ThreadStakeMinter started\n");
    CWallet* pwallet = (CWallet*)parg;
    try
    {
        vnThreadsRunning[THREAD_MINTER]++;
        BitcoinMiner(pwallet, true);
        vnThreadsRunning[THREAD_MINTER]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_MINTER]--;
        PrintException(&e, "ThreadStakeMinter()");
    } catch (...) {
        vnThreadsRunning[THREAD_MINTER]--;
        PrintException(NULL, "ThreadStakeMinter()");
    }
    printf("ThreadStakeMinter exiting, %d threads remaining\n", vnThreadsRunning[THREAD_MINTER]);
}






bool BindListenPort(string& strError)
{
    strError = "";
    int nOne = 1;
    addrLocalHost.SetPort(GetListenPort());

#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR)
    {
        strError = strprintf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);
        printf("%s\n", strError.c_str());
        return false;
    }
#endif

    // Create socket for listening for incoming connections
    hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

#ifdef SO_NOSIGPIPE
    // Different way of disabling SIGPIPE on BSD
    setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif

#ifndef WIN32
    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.  Not an issue on windows.
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
#endif

#ifdef WIN32
    // Set to nonblocking, incoming connections will also inherit this
    if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
    if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
    {
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY; // bind to all IPs on this computer
    sockaddr.sin_port = htons(GetListenPort());
    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to port %d on this computer.  PPCoin is probably already running."), ntohs(sockaddr.sin_port));
        else
            strError = strprintf("Error: Unable to bind to port %d on this computer (bind returned error %d)", ntohs(sockaddr.sin_port), nErr);
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("Bound to port %d\n", ntohs(sockaddr.sin_port));

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
        printf("%s\n", strError.c_str());
        return false;
    }

    return true;
}

void StartNode(void* parg)
{
    if (semOutbound == NULL) {
        // initialize semaphore
        int nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, (int)GetArg("-maxconnections", 125));
        semOutbound = new CSemaphore(nMaxOutbound);
    }

#ifdef USE_UPNP
#if USE_UPNP
    fUseUPnP = GetBoolArg("-upnp", true);
#else
    fUseUPnP = GetBoolArg("-upnp", false);
#endif
#endif

    if (pnodeLocalHost == NULL)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));

#ifdef WIN32
    // Get local host ip
    char pszHostName[1000] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
    {
        vector<CNetAddr> vaddr;
        if (LookupHost(pszHostName, vaddr))
        {
            BOOST_FOREACH (const CNetAddr &addr, vaddr)
            {
                if (!addr.IsLocal())
                {
                    addrLocalHost.SetIP(addr);
                    break;
                }
            }
        }
    }
#else
    // Get local host ip
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0)
    {
        for (struct ifaddrs* ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            if (strcmp(ifa->ifa_name, "lo0") == 0) continue;
            char pszIP[100];
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                if (inet_ntop(ifa->ifa_addr->sa_family, (void*)&(s4->sin_addr), pszIP, sizeof(pszIP)) != NULL)
                    printf("ipv4 %s: %s\n", ifa->ifa_name, pszIP);

                // Take the first IP that isn't loopback 127.x.x.x
                CAddress addr(CService(s4->sin_addr, GetListenPort()), nLocalServices);
                if (addr.IsValid() && !addr.IsLocal())
                {
                    addrLocalHost = addr;
                    break;
                }
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                if (inet_ntop(ifa->ifa_addr->sa_family, (void*)&(s6->sin6_addr), pszIP, sizeof(pszIP)) != NULL)
                    printf("ipv6 %s: %s\n", ifa->ifa_name, pszIP);
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
    printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());

    if (fUseProxy || mapArgs.count("-connect") || fNoListen)
    {
        // Proxies can't take incoming connections
        addrLocalHost.SetIP(CNetAddr("0.0.0.0"));
        printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());
    }
    else
    {
        CreateThread(ThreadGetMyExternalIP, NULL);
    }

    //
    // Start threads
    //

    if (!GetBoolArg("-dnsseed", true))
        printf("DNS seeding disabled\n");
    else
        if (!CreateThread(ThreadDNSAddressSeed, NULL))
            printf("Error: CreateThread(ThreadDNSAddressSeed) failed\n");

    // Map ports with UPnP
    if (fHaveUPnP)
        MapPort(fUseUPnP);

    // Get addresses from IRC and advertise ours
    // if (!CreateThread(ThreadIRCSeed, NULL))
    //     printf("Error: CreateThread(ThreadIRCSeed) failed\n");
    // IRC disabled with ppcoin
    printf("IRC seeding/communication disabled\n");

    // Send and receive from sockets, accept connections
    if (!CreateThread(ThreadSocketHandler, NULL))
        printf("Error: CreateThread(ThreadSocketHandler) failed\n");

    // Initiate outbound connections from -addnode
    if (!CreateThread(ThreadOpenAddedConnections, NULL))
        printf("Error: CreateThread(ThreadOpenAddedConnections) failed\n");

    // Initiate outbound connections
    if (!CreateThread(ThreadOpenConnections, NULL))
        printf("Error: CreateThread(ThreadOpenConnections) failed\n");

    // Process messages
    if (!CreateThread(ThreadMessageHandler, NULL))
        printf("Error: CreateThread(ThreadMessageHandler) failed\n");

    // Dump network addresses
    if (!CreateThread(ThreadDumpAddress, NULL))
        printf("Error; CreateThread(ThreadDumpAddress) failed\n");

    // Generate coins in the background
    GenerateBitcoins(GetBoolArg("-gen", false), pwalletMain);

    // ppcoin: mint proof-of-stake blocks in the background
    if (!CreateThread(ThreadStakeMinter, pwalletMain))
        printf("Error: CreateThread(ThreadStakeMinter) failed\n");
}

bool StopNode()
{
    printf("StopNode()\n");
    fShutdown = true;
    nTransactionsUpdated++;
    int64 nStart = GetTime();
    if (semOutbound)
        for (int i=0; i<MAX_OUTBOUND_CONNECTIONS; i++)
            semOutbound->post();
    do
    {
        int nThreadsRunning = 0;
        for (int n = 0; n < THREAD_MAX; n++)
            nThreadsRunning += vnThreadsRunning[n];
        if (nThreadsRunning == 0)
            break;
        if (GetTime() - nStart > 20)
            break;
        Sleep(20);
    } while(true);
    if (vnThreadsRunning[THREAD_SOCKETHANDLER] > 0) printf("ThreadSocketHandler still running\n");
    if (vnThreadsRunning[THREAD_OPENCONNECTIONS] > 0) printf("ThreadOpenConnections still running\n");
    if (vnThreadsRunning[THREAD_MESSAGEHANDLER] > 0) printf("ThreadMessageHandler still running\n");
    if (vnThreadsRunning[THREAD_MINER] > 0) printf("ThreadBitcoinMiner still running\n");
    if (vnThreadsRunning[THREAD_RPCSERVER] > 0) printf("ThreadRPCServer still running\n");
    if (fHaveUPnP && vnThreadsRunning[THREAD_UPNP] > 0) printf("ThreadMapPort still running\n");
    if (vnThreadsRunning[THREAD_DNSSEED] > 0) printf("ThreadDNSAddressSeed still running\n");
    if (vnThreadsRunning[THREAD_ADDEDCONNECTIONS] > 0) printf("ThreadOpenAddedConnections still running\n");
    if (vnThreadsRunning[THREAD_DUMPADDRESS] > 0) printf("ThreadDumpAddresses still running\n");
    if (vnThreadsRunning[THREAD_MINTER] > 0) printf("ThreadStakeMinter still running\n");
    while (vnThreadsRunning[THREAD_MESSAGEHANDLER] > 0 || vnThreadsRunning[THREAD_RPCSERVER] > 0)
        Sleep(20);
    Sleep(50);
    DumpAddresses();
    return true;
}

class CNetCleanup
{
public:
    CNetCleanup()
    {
    }
    ~CNetCleanup()
    {
        // Close sockets
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode->hSocket != INVALID_SOCKET)
                closesocket(pnode->hSocket);
        if (hListenSocket != INVALID_SOCKET)
            if (closesocket(hListenSocket) == SOCKET_ERROR)
                printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;
