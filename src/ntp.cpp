#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#ifndef WIN32
#include <unistd.h>
#endif

#include "netbase.h"
#include "net.h"
//#include "util.h"
#include "ui_interface.h"

extern int GetRandInt(int nMax);

/*
 * NTP uses two fixed point formats.  The first (l_fp) is the "long"
 * format and is 64 bits long with the decimal between bits 31 and 32.
 * This is used for time stamps in the NTP packet header (in network
 * byte order) and for internal computations of offsets (in local host
 * byte order). We use the same structure for both signed and unsigned
 * values, which is a big hack but saves rewriting all the operators
 * twice. Just to confuse this, we also sometimes just carry the
 * fractional part in calculations, in both signed and unsigned forms.
 * Anyway, an l_fp looks like:
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Integral Part                         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Fractional Part                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * REF http://www.eecis.udel.edu/~mills/database/rfc/rfc2030.txt
 */


typedef struct {
  union {
    uint32_t Xl_ui;
    int32_t Xl_i;
  } Ul_i;
  union {
    uint32_t Xl_uf;
    int32_t Xl_f;
  } Ul_f;
} l_fp;


inline void Ntp2Unix(const uint32_t &n, time_t &u) {
    // Ntp's time scale starts in 1900, Unix in 1970.

    u = n - 0x83aa7e80; // 2208988800 1970 - 1900 in seconds
}

inline void ntohl_fp(l_fp *n, l_fp *h) {
    (h)->Ul_i.Xl_ui = ntohl((n)->Ul_i.Xl_ui);
    (h)->Ul_f.Xl_uf = ntohl((n)->Ul_f.Xl_uf);
}

struct pkt {
  uint8_t  li_vn_mode;     /* leap indicator, version and mode */
  uint8_t  stratum;        /* peer stratum */
  uint8_t  ppoll;          /* peer poll interval */
  int8_t  precision;      /* peer clock precision */
  uint32_t    rootdelay;      /* distance to primary clock */
  uint32_t    rootdispersion; /* clock dispersion */
  uint32_t refid;          /* reference clock ID */
  l_fp    ref;        /* time peer clock was last updated */
  l_fp    org;            /* originate time stamp */
  l_fp    rec;            /* receive time stamp */
  l_fp    xmt;            /* transmit time stamp */

  uint32_t exten[1];       /* misused */
  uint8_t  mac[5 * sizeof(uint32_t)]; /* mac */
};

const int nServersCount = 162;

std::string NtpServers[162] = {
    // Microsoft
    "time.windows.com",

    // Google
    "time1.google.com",
    "time2.google.com",
    "time3.google.com",
    "time4.google.com",

    // Hurricane Electric
    "clock.sjc.he.net",
    "clock.nyc.he.net",

    // SixXS
    "ntp.sixxs.net",
    "ntp.eu.sixxs.net",
    "ntp.us.sixxs.net",
    "ntp.ap.sixxs.net",

    // Russian Federation
    "ntp.karelia.pro",
    "ntp.alpet.me",
    "aviel.alpet.me",
    "ntp.sampo.ru",
    "ntp.szt.ru",
    "ntp.ix.ru",
    "ntp1.stratum2.ru",
    "ntp2.stratum2.ru",
    "ntp3.stratum2.ru",
    "ntp4.stratum2.ru",
    "ntp5.stratum2.ru",
    "ntp6.stratum2.ru",
    "ntp7.stratum2.ru",
    "ntp1.stratum1.ru",
    "ntp2.stratum1.ru",
    "ntp3.stratum1.ru",
    "ntp4.stratum1.ru",
    "ntp1.vniiftri.ru",
    "ntp2.vniiftri.ru",
    "ntp3.vniiftri.ru",
    "ntp4.vniiftri.ru",
    "ntp21.vniiftri.ru",
    "ntp1.niiftri.irkutsk.ru",
    "ntp2.niiftri.irkutsk.ru",
    "vniiftri.khv.ru",
    "vniiftri2.khv.ru",
    "ntp0.zenon.net",
    "ntp.mobatime.ru",
    "0.ru.pool.ntp.org",
    "1.ru.pool.ntp.org",
    "2.ru.pool.ntp.org",
    "3.ru.pool.ntp.org",

    // United States
    "tock.cs.unlv.edu",
    "timex.cs.columbia.edu",
    "tick.cs.unlv.edu",
    "sundial.columbia.edu",
    "ntp-1.ece.cmu.edu",
    "ntp-2.ece.cmu.edu",
    "ntp1.cs.wisc.edu",
    "ntp2.cs.wisc.edu",
    "ntp3.cs.wisc.edu",
    "ntp-01.caltech.edu",
    "ntp-02.caltech.edu",
    "ntp-03.caltech.edu",
    "ntp-04.caltech.edu",
    "nist1-pa.ustiming.org",
    "time.nist.gov",
    "time-a.nist.gov",
    "time-b.nist.gov",
    "time-c.nist.gov",
    "time-d.nist.gov",
    "time-nw.nist.gov",
    "nist1-macon.macon.ga.us",
    "nist.netservicesgroup.com",
    "nisttime.carsoncity.k12.mi.us",
    "nist1-lnk.binary.net",
    "wwv.nist.gov",
    "time-a.timefreq.bldrdoc.gov",
    "time-b.timefreq.bldrdoc.gov",
    "time-c.timefreq.bldrdoc.gov",
    "utcnist.colorado.edu",
    "utcnist2.colorado.edu",
    "ntp-nist.ldsbc.net",
    "nist1-lv.ustiming.org",
    "time-nw.nist.gov",
    "nist-time-server.eoni.com",
    "nist-time-server.eoni.com",
    "ntp1.bu.edu",
    "ntp2.bu.edu",
    "ntp3.bu.edu",
    "0.us.pool.ntp.org",
    "1.us.pool.ntp.org",
    "2.us.pool.ntp.org",
    "3.us.pool.ntp.org",
    "wwv.otc.psu.edu",
    "otc1.psu.edu",
    "otc2.psu.edu",
    "now.okstate.edu",
    "ntp.colby.edu",
    "bonehed.lcs.mit.edu",
    "ntp-s1.cise.ufl.edu",

    // South Africa
    "ntp1.meraka.csir.co.za",
    "ntp.is.co.za",
    "ntp2.is.co.za",
    "igubu.saix.net",
    "ntp1.neology.co.za",
    "ntp2.neology.co.za",
    "tick.meraka.csir.co.za",
    "tock.meraka.csir.co.za",
    "ntp.time.org.za",
    "ntp1.meraka.csir.co.za",
    "ntp2.meraka.csir.co.za",
    "0.za.pool.ntp.org",
    "1.za.pool.ntp.org",
    "2.za.pool.ntp.org",
    "3.za.pool.ntp.org",

    // Italy
    "ntp0.ien.it",
    "ntp1.ien.it",
    "ntp2.ien.it",
    "ntp1.inrim.it",
    "ntp2.inrim.it",
    "0.it.pool.ntp.org",
    "1.it.pool.ntp.org",
    "2.it.pool.ntp.org",
    "3.it.pool.ntp.org",

    // Netherlands
    "ntp0.nl.net",
    "ntp1.nl.net",
    "ntp2.nl.net",
    "ntp.utwente.nl",
    "0.nl.pool.ntp.org",
    "1.nl.pool.ntp.org",
    "2.nl.pool.ntp.org",
    "3.nl.pool.ntp.org",

    // United Kingdom
    "ntp2d.mcc.ac.uk",
    "ntp2c.mcc.ac.uk",
    "ntp2b.mcc.ac.uk",
    "ntp.exnet.com",
    "ntp.cis.strath.ac.uk",
    "ntppub.le.ac.uk",
    "0.uk.pool.ntp.org",
    "1.uk.pool.ntp.org",
    "2.uk.pool.ntp.org",
    "3.uk.pool.ntp.org",

    // Canada
    "chime.utoronto.ca",
    "tick.utoronto.ca",
    "time.nrc.ca",
    "timelord.uregina.ca",
    "tock.utoronto.ca",
    "www1.cmc.ec.gc.ca",
    "www2.cmc.ec.gc.ca",
    "0.ca.pool.ntp.org",
    "1.ca.pool.ntp.org",
    "2.ca.pool.ntp.org",
    "3.ca.pool.ntp.org",

    // Japan
    "ntp.nict.jp",
    "0.jp.pool.ntp.org",
    "1.jp.pool.ntp.org",
    "2.jp.pool.ntp.org",
    "3.jp.pool.ntp.org",

    // Australia
    "ntp.cs.mu.oz.au",
    "augean.eleceng.adelaide.edu.au",
    "0.au.pool.ntp.org",
    "1.au.pool.ntp.org",
    "2.au.pool.ntp.org",
    "3.au.pool.ntp.org",

    // Slovenia
    "time.ijs.si",

    // Austria
    "0.at.pool.ntp.org",
    "1.at.pool.ntp.org",
    "2.at.pool.ntp.org",
    "3.at.pool.ntp.org",

    // ???
    "clepsydra.dec.com",

    // ... To be continued
};

bool InitWithHost(const std::string &strHostName, SOCKET &sockfd, socklen_t &servlen, struct sockaddr *pcliaddr) {
  
    sockfd = INVALID_SOCKET;

    std::vector<CNetAddr> vIP;
    bool fRet = LookupHost(strHostName.c_str(), vIP, 10, true);
    if (!fRet) {
        return false;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(123);

    bool found = false;
    for(unsigned int i = 0; i < vIP.size(); i++) {
        if ((found = vIP[i].GetInAddr(&servaddr.sin_addr)) != false) {
            break;
        }
    }

    if (!found) {
        return false;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == INVALID_SOCKET)
        return false; // socket initialization error

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1 ) {
        return false; // "connection" error
    }


    *pcliaddr = *((struct sockaddr *) &servaddr);
    servlen = sizeof(servaddr);

    return true;
}

bool InitWithRandom(SOCKET &sockfd, socklen_t &servlen, struct sockaddr *pcliaddr) {

    for (int nAttempt = 0; nAttempt < nServersCount; nAttempt++) {
        int nServerNum = GetRandInt(nServersCount);
        if (InitWithHost(NtpServers[nServerNum], sockfd, servlen, pcliaddr)) {
            return true;
        }
    }

    return false;
}

int64_t DoReq(SOCKET sockfd, socklen_t servlen, struct sockaddr cliaddr) {


#ifdef WIN32
    u_long nOne = 1;
    if (ioctlsocket(sockfd, FIONBIO, &nOne) == SOCKET_ERROR) {
        printf("ConnectSocket() : ioctlsocket non-blocking setting failed, error %d\n", WSAGetLastError());
#else
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == SOCKET_ERROR) {
        printf("ConnectSocket() : fcntl non-blocking setting failed, error %d\n", errno);
#endif
        return -2;
    }

    struct timeval timeout = {10, 0};
    struct pkt *msg = new pkt;
    struct pkt *prt  = new pkt;
    time_t seconds_transmit;
    int len = 48;

    msg->li_vn_mode=227;
    msg->stratum=0;
    msg->ppoll=4;
    msg->precision=0;
    msg->rootdelay=0;
    msg->rootdispersion=0;

    msg->ref.Ul_i.Xl_i=0;
    msg->ref.Ul_f.Xl_f=0;
    msg->org.Ul_i.Xl_i=0;
    msg->org.Ul_f.Xl_f=0;
    msg->rec.Ul_i.Xl_i=0;
    msg->rec.Ul_f.Xl_f=0;
    msg->xmt.Ul_i.Xl_i=0;
    msg->xmt.Ul_f.Xl_f=0;

    int retcode = sendto(sockfd, (char *) msg, len, 0, &cliaddr, servlen);
    if (retcode < 0) {
        printf("sendto() failed: %d\n", retcode);
        seconds_transmit = -3;
        goto _end;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);

    retcode = select(sockfd + 1, &fdset, NULL, NULL, &timeout);
    if (retcode <= 0) {
        printf("recvfrom() error\n");
        seconds_transmit = -4;
        goto _end;
    }

    recvfrom(sockfd, (char *) msg, len, 0, NULL, NULL);
    ntohl_fp(&msg->xmt, &prt->xmt);
    Ntp2Unix(prt->xmt.Ul_i.Xl_ui, seconds_transmit);

    _end:

    delete msg;
    delete prt;

    return seconds_transmit;
}

int64_t NtpGetTime(CNetAddr& ip) {
    struct sockaddr cliaddr;

    SOCKET sockfd;
    socklen_t servlen;

    if (!InitWithRandom(sockfd, servlen, &cliaddr))
        return -1;

    ip = CNetAddr(((sockaddr_in *)&cliaddr)->sin_addr);
    int64_t nTime = DoReq(sockfd, servlen, cliaddr);

    CloseSocket(sockfd);

    return nTime;
}

int64_t NtpGetTime(const std::string &strHostName)
{
    struct sockaddr cliaddr;

    SOCKET sockfd;
    socklen_t servlen;

    if (!InitWithHost(strHostName, sockfd, servlen, &cliaddr))
        return -1;

    int64_t nTime = DoReq(sockfd, servlen, cliaddr);

    CloseSocket(sockfd);

    return nTime;
}

// NTP server, which we unconditionally trust. This may be your own installation of ntpd somewhere, for example. 
// "localhost" means "trust no one"
std::string strTrustedUpstream = "localhost";

// Current offset
int64_t nNtpOffset = INT64_MAX;

int64_t GetNtpOffset() {
    return nNtpOffset;
}

void ThreadNtpSamples(void* parg) {
    const int64_t nMaxOffset = nOneDay; // Not a real limit, just sanity threshold.

    printf("Trying to find NTP server at localhost...\n");

    std::string strLocalHost = "127.0.0.1";
    if (NtpGetTime(strLocalHost) == GetTime()) {
        printf("There is NTP server active at localhost,  we don't need NTP thread.\n");

        nNtpOffset = 0;
        return;
    }

    printf("ThreadNtpSamples started\n");
    vnThreadsRunning[THREAD_NTP]++;

    // Make this thread recognisable as time synchronization thread
    RenameThread("novacoin-ntp-samples");

    CMedianFilter<int64_t> vTimeOffsets(200,0);

    while (!fShutdown) {
        if (strTrustedUpstream != "localhost") {
            // Trying to get new offset sample from trusted NTP server.
            int64_t nClockOffset = NtpGetTime(strTrustedUpstream) - GetTime();

            if (abs64(nClockOffset) < nMaxOffset) {
                // Everything seems right, remember new trusted offset.
                printf("ThreadNtpSamples: new offset sample from %s, offset=%" PRId64 ".\n", strTrustedUpstream.c_str(), nClockOffset);
                nNtpOffset = nClockOffset;
            }
            else {
                // Something went wrong, disable trusted offset sampling.
                nNtpOffset = INT64_MAX;
                strTrustedUpstream = "localhost";

                int nSleepMinutes = 1 + GetRandInt(9); // Sleep for 1-10 minutes.
                for (int i = 0; i < nSleepMinutes * 60 && !fShutdown; i++)
                    Sleep(1000);

                continue;
            }
        }
        else {
            // Now, trying to get 2-4 samples from random NTP servers.
            int nSamplesCount = 2 + GetRandInt(2);

            for (int i = 0; i < nSamplesCount; i++) {
                CNetAddr ip;
                int64_t nClockOffset = NtpGetTime(ip) - GetTime();

                if (abs64(nClockOffset) < nMaxOffset) { // Skip the deliberately wrong timestamps
                    printf("ThreadNtpSamples: new offset sample from %s, offset=%" PRId64 ".\n", ip.ToString().c_str(), nClockOffset);
                    vTimeOffsets.input(nClockOffset);
                }
            }

            if (vTimeOffsets.size() > 1) {
                nNtpOffset = vTimeOffsets.median();
            }
            else {
                // Not enough offsets yet, try to collect additional samples later.
                nNtpOffset = INT64_MAX;
                int nSleepMinutes = 1 + GetRandInt(4); // Sleep for 1-5 minutes.
                for (int i = 0; i < nSleepMinutes * 60 && !fShutdown; i++) 
                    Sleep(1000);
                continue;
            }
        }

        if (GetNodesOffset() == INT_MAX && abs64(nNtpOffset) > 40 * 60)
        {
            // If there is not enough node offsets data and NTP time offset is greater than 40 minutes then give a warning.
            std::string strMessage = _("Warning: Please check that your computer's date and time are correct! If your clock is wrong NovaCoin will not work properly.");
            strMiscWarning = strMessage;
            printf("*** %s\n", strMessage.c_str());
            uiInterface.ThreadSafeMessageBox(strMessage+" ", std::string("NovaCoin"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION);
        }

        printf("nNtpOffset = %+" PRId64 "  (%+" PRId64 " minutes)\n", nNtpOffset, nNtpOffset/60);

        int nSleepHours = 1 + GetRandInt(5); // Sleep for 1-6 hours.
        for (int i = 0; i < nSleepHours * 3600 && !fShutdown; i++)
            Sleep(1000);
    }

    vnThreadsRunning[THREAD_NTP]--;
    printf("ThreadNtpSamples exited\n");
}
