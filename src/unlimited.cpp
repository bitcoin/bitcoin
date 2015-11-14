// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "util.h"
#include "unlimited.h"
#include "clientversion.h"
#include "rpcserver.h"
#include "utilstrencodings.h"
#include "tinyformat.h"
#include "leakybucket.h"
#include "primitives/block.h"
#include "chain.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

enum { DEFAULT_EXCESSIVE_ACCEPT_DEPTH = 6, DEFAULT_EXCESSIVE_BLOCK_SIZE = 100000 };  // 16*1024*1024 };

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;
unsigned int excessiveBlockSize = DEFAULT_EXCESSIVE_BLOCK_SIZE;
unsigned int excessiveAcceptDepth =  DEFAULT_EXCESSIVE_ACCEPT_DEPTH;

// Variables for traffic shaping
CLeakyBucket receiveShaper(DEFAULT_MAX_RECV_BURST, DEFAULT_AVE_RECV);
CLeakyBucket sendShaper(DEFAULT_MAX_SEND_BURST, DEFAULT_AVE_SEND);
boost::chrono::steady_clock CLeakyBucket::clock;


void UnlimitedSetup(void)
{
  maxGeneratedBlock = GetArg("-blockmaxsize", DEFAULT_MAX_GENERATED_BLOCK_SIZE);

  //  Init network shapers
  int64_t rb = GetArg("-receiveburst", DEFAULT_MAX_RECV_BURST);
  // parameter is in KBytes/sec, leaky bucket is in bytes/sec.  But if it is "off" then don't multiply
  if (rb != LONG_LONG_MAX)
    rb *= 1024;
  int64_t ra = GetArg("-receiveavg", DEFAULT_MAX_RECV_BURST);
  if (ra != LONG_LONG_MAX)
    ra *= 1024;
  int64_t sb = GetArg("-sendburst", DEFAULT_MAX_RECV_BURST);
  if (sb != LONG_LONG_MAX)
    sb *= 1024;
  int64_t sa = GetArg("-sendavg", DEFAULT_MAX_RECV_BURST);
  if (sa != LONG_LONG_MAX)
    sa *= 1024;

  receiveShaper.set(rb, ra);
  sendShaper.set(sb, sa);
}



std::string LicenseInfo()
{
    return FormatParagraph(strprintf(_("Copyright (C) 2009-%i The Bitcoin Unlimited Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin Core Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin XT Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           "\n" +
           FormatParagraph(_("This is experimental software.")) + "\n" +
           "\n" +
           FormatParagraph(_("Distributed under the MIT software license, see the accompanying file COPYING or <http://www.opensource.org/licenses/mit-license.php>.")) + "\n" +
           "\n" +
           FormatParagraph(_("This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit <https://www.openssl.org/> and cryptographic software written by Eric Young and UPnP software written by Thomas Bernard.")) +
           "\n";
}

int isChainExcessive(const CBlockIndex* blk,unsigned int goBack)
{
  if (goBack == 0) goBack = excessiveAcceptDepth;
  for (unsigned int i=1;i<=goBack;i++, blk=blk->pprev)
    {
      if (!blk) return 0;  // Not excessive if we hit the beginning
      if (blk->nStatus & BLOCK_EXCESSIVE) return i;
    }
  return 0;
}

bool CheckExcessive(const CBlock& block,uint64_t blockSize, uint64_t nSigOps,uint64_t nTx)
{
  if (blockSize > excessiveBlockSize) 
    {
      LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many bytes\n", block.nVersion, block.nTime, blockSize, nTx,nSigOps);
      return true;
    }
  
  LogPrintf("Acceptable block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d\n", block.nVersion, block.nTime, blockSize, nTx,nSigOps);  
  return false;
}


UniValue gettrafficshaping(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() == 1) {
        strCommand = params[0].get_str();
    }

    if (fHelp || (params.size() != 0))
        throw runtime_error(
            "gettrafficshaping"
            "\nReturns the current settings for the network send and receive bandwidth and burst in kilobytes per second.\n"
            "\nArguments: None\n"
            "\nResult:\n"
            "  {\n"
            "    \"sendBurst\" : 40,   (string) The maximum send bandwidth in Kbytes/sec\n"
            "    \"sendAve\" : 30,   (string) The average send bandwidth in Kbytes/sec\n"
            "    \"recvBurst\" : 20,   (string) The maximum receive bandwidth in Kbytes/sec\n"
            "    \"recvAve\" : 10,   (string) The average receive bandwidth in Kbytes/sec\n"
            "  }\n"
            "\n NOTE: if the send and/or recv parameters do not exist, shaping in that direction is disabled.\n"
            "\nExamples:\n" +
            HelpExampleCli("gettrafficshaping", "") + HelpExampleRpc("gettrafficshaping", ""));

    UniValue ret(UniValue::VARR);
    int64_t max, ave;
    sendShaper.get(&max, &ave);
    if (ave != LONG_MAX) {
        ret.push_back(Pair("sendBurst", max / 1024));
        ret.push_back(Pair("sendAve", ave / 1024));
    }
    receiveShaper.get(&max, &ave);
    if (ave != LONG_MAX) {
        ret.push_back(Pair("recvBurst", max / 1024));
        ret.push_back(Pair("recvAve", ave / 1024));
    }
    return ret;
}

UniValue settrafficshaping(const UniValue& params, bool fHelp)
{
    bool disable = false;
    bool badArg = false;
    string strCommand;
    CLeakyBucket* bucket = NULL;
    if (params.size() >= 2) {
        strCommand = params[0].get_str();
        if (strCommand == "send")
            bucket = &sendShaper;
        if (strCommand == "receive")
            bucket = &receiveShaper;
        if (strCommand == "recv")
            bucket = &receiveShaper;
    }
    if (params.size() == 2) {
        if (params[1].get_str() == "disable")
            disable = true;
        else
            badArg = true;
    } else if (params.size() != 3)
        badArg = true;

    if (fHelp || badArg || bucket == NULL)
        throw runtime_error(
            "settrafficshaping \"send|receive\" \"burstKB\" \"averageKB\""
            "\nSets the network send or receive bandwidth and burst in kilobytes per second.\n"
            "\nArguments:\n"
            "1. \"send|receive\"     (string, required) Are you setting the transmit or receive bandwidth\n"
            "2. \"burst\"  (integer, required) Specify the maximum burst size in Kbytes/sec (actual max will be 1 packet larger than this number)\n"
            "2. \"average\"  (integer, required) Specify the average throughput in Kbytes/sec\n"
            "\nExamples:\n" +
            HelpExampleCli("settrafficshaping", "\"receive\" 10000 1024") + HelpExampleCli("settrafficshaping", "\"receive\" disable") + HelpExampleRpc("settrafficshaping", "\"receive\" 10000 1024"));

    if (disable) {
        if (bucket)
            bucket->disable();
    } else {
        uint64_t burst;
        uint64_t ave;
        if (params[1].isNum())
            burst = params[1].get_int64();
        else {
            string temp = params[1].get_str();
            burst = boost::lexical_cast<uint64_t>(temp);
        }
        if (params[2].isNum())
            ave = params[2].get_int64();
        else {
            string temp = params[2].get_str();
            ave = boost::lexical_cast<uint64_t>(temp);
        }
        if (burst < ave) {
            throw runtime_error("Burst rate must be greater than the average rate"
                                "\nsettrafficshaping \"send|receive\" \"burst\" \"average\"");
        }

        bucket->set(burst * 1024, ave * 1024);
    }

    return NullUniValue;
}
