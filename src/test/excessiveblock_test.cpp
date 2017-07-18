#include "unlimited.h"

#include "test/test_bitcoin.h"
#include "../consensus/consensus.h"

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

// Defined in rpc_tests.cpp not bitcoin-cli.cpp
extern UniValue CallRPC(string strMethod);

BOOST_FIXTURE_TEST_SUITE(excessiveblock_test, TestingSetup)

BOOST_AUTO_TEST_CASE(rpc_excessive)
{
    BOOST_CHECK_NO_THROW(CallRPC("getexcessiveblock"));

    BOOST_CHECK_NO_THROW(CallRPC("getminingmaxblock"));

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock not_uint"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000000 not_uint"), boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000000 -1"), boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setexcessiveblock -1 0"), boost::bad_lexical_cast);

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000 1"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("setminingmaxblock 1000"));
    BOOST_CHECK_NO_THROW(CallRPC("setexcessiveblock 1000 1"));

    BOOST_CHECK_THROW(CallRPC("setexcessiveblock 1000 0 0"), runtime_error);

    BOOST_CHECK_THROW(CallRPC("setminingmaxblock"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock 100000"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock not_uint"),  boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock -1"),  boost::bad_lexical_cast);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock 0"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("setminingmaxblock 0 0"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("setminingmaxblock 1000"));
    BOOST_CHECK_NO_THROW(CallRPC("setminingmaxblock 101"));
    
}

BOOST_AUTO_TEST_CASE(buip005)
{
    string exceptedEB;
    string exceptedAD;
    excessiveBlockSize = 1000000;
    excessiveAcceptDepth = 9999999;
    exceptedEB = "EB1";
    exceptedAD = "AD9999999";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    BOOST_CHECK_MESSAGE(BUComments.back() == exceptedAD,
                        "AD ought to have been " << exceptedAD << " when excessiveBlockSize = " << excessiveAcceptDepth);
    excessiveBlockSize = 100000;
    excessiveAcceptDepth = 9999999 + 1;
    exceptedEB = "EB0.1";
    exceptedAD = "AD9999999";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    BOOST_CHECK_MESSAGE(BUComments.back() == exceptedAD,
                        "AD ought to have been " << exceptedAD << " when excessiveBlockSize = " << excessiveAcceptDepth);
    excessiveBlockSize = 10000;
    exceptedEB = "EB0";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 1670000;
    exceptedEB = "EB1.6";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 150000;
    exceptedEB = "EB0.1";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 0;
    exceptedEB = "EB0";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 3800000000;
    exceptedEB = "EB3800";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    excessiveBlockSize = 49200000000;
    exceptedEB = "EB49200";
    settingsToUserAgentString();
    BOOST_CHECK_MESSAGE(BUComments.front() == exceptedEB,
                        "EB ought to have been rounded to " << exceptedEB << " when excessiveBlockSize = "
                        << excessiveBlockSize << " but was " << BUComments.front());
    // set back to defaults
    excessiveBlockSize = 1000000;
    excessiveAcceptDepth = 4;
}


BOOST_AUTO_TEST_CASE(excessiveChecks)
{
  CBlock block;

  excessiveBlockSize = 16000000;  // Ignore excessive block size when checking sigops and block effort

  // Check sigops values

  // Maintain compatibility with the old sigops calculator for blocks <= 1MB
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE-1,BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS,100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE-1,BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS,100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE,BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS,100,100), "improper sigops");

  BOOST_CHECK_MESSAGE(true == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE-1,BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS+1,100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(true == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE,BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS+1,100,100), "improper sigops");


  // Check sigops > 1MB.
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,1000000+1,(blockSigopsPerMb.value*2),100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(true == CheckExcessive(block,1000000+1,(blockSigopsPerMb.value*2)+1,100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(true == CheckExcessive(block,(2*1000000),(blockSigopsPerMb.value*2)+1,100,100), "improper sigops");
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,(2*1000000)+1,(blockSigopsPerMb.value*2)+1,100,100), "improper sigops");

  
  // Check tx size values
  maxTxSize.value = DEFAULT_LARGEST_TRANSACTION;

  // Within a 1 MB block, a 1MB transaction is not excessive
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE,1,1,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE), "improper max tx");

  // With a > 1 MB block, use the maxTxSize to determine
  BOOST_CHECK_MESSAGE(false == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE+1,1,1,maxTxSize.value), "improper max tx");
  BOOST_CHECK_MESSAGE(true == CheckExcessive(block,BLOCKSTREAM_CORE_MAX_BLOCK_SIZE+1,1,1,maxTxSize.value+1), "improper max tx");


}

BOOST_AUTO_TEST_CASE(check_validator_rule)
{
    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(1000000, 1000000));
    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(16000000, 1000000));
    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(1000001, 1000000));

    BOOST_CHECK( ! MiningAndExcessiveBlockValidatorRule(1000000, 1000001));
    BOOST_CHECK( ! MiningAndExcessiveBlockValidatorRule(1000000, 16000000));

    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(1357, 1357));
    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(161616, 2222));
    BOOST_CHECK( MiningAndExcessiveBlockValidatorRule(88889, 88888));

    BOOST_CHECK( ! MiningAndExcessiveBlockValidatorRule(929292, 929293));
    BOOST_CHECK( ! MiningAndExcessiveBlockValidatorRule(4, 234245));
}

BOOST_AUTO_TEST_CASE(check_excessive_validator)
{
    uint64_t c_mgb = maxGeneratedBlock;
    uint64_t c_ebs = excessiveBlockSize;

    // fudge global variables....
    maxGeneratedBlock = 1000000;
    excessiveBlockSize = 888;

    unsigned int tmpExcessive = 1000000;
    std::string str;

    str = ExcessiveBlockValidator(tmpExcessive, NULL, true);
    BOOST_CHECK(str.empty());

    excessiveBlockSize = 888;
    str = ExcessiveBlockValidator(tmpExcessive, NULL, false);
    BOOST_CHECK(str.empty());

    str = ExcessiveBlockValidator(tmpExcessive, (uint64_t *) 42, true);
    BOOST_CHECK(str.empty());

    tmpExcessive = maxGeneratedBlock + 1;

    str = ExcessiveBlockValidator(tmpExcessive, NULL, true);
    BOOST_CHECK(str.empty());

    excessiveBlockSize = 888;
    str = ExcessiveBlockValidator(tmpExcessive, NULL, false);
    BOOST_CHECK(str.empty());

    str = ExcessiveBlockValidator(tmpExcessive, (uint64_t *) 42, true);
    BOOST_CHECK(str.empty());

    tmpExcessive = maxGeneratedBlock - 1;

    str = ExcessiveBlockValidator(tmpExcessive, NULL, true);
    BOOST_CHECK(! str.empty());

    str = ExcessiveBlockValidator(tmpExcessive, NULL, false);
    BOOST_CHECK(str.empty());

    str = ExcessiveBlockValidator(tmpExcessive, (uint64_t *) 42, true);
    BOOST_CHECK(! str.empty());

    maxGeneratedBlock = c_mgb;
    excessiveBlockSize = c_ebs;
}

BOOST_AUTO_TEST_CASE(check_generated_block_validator)
{
    uint64_t c_mgb = maxGeneratedBlock;
    uint64_t c_ebs = excessiveBlockSize;

    // fudge global variables....
    maxGeneratedBlock = 888;
    excessiveBlockSize = 1000000;

    uint64_t tmpMGB = 1000000;
    std::string str;

    str = MiningBlockSizeValidator(tmpMGB, NULL, true);
    BOOST_CHECK(str.empty());

    maxGeneratedBlock = 8888881;
    str = MiningBlockSizeValidator(tmpMGB, NULL, false);
    BOOST_CHECK(str.empty());

    str = MiningBlockSizeValidator(tmpMGB, (uint64_t *) 42, true);
    BOOST_CHECK(str.empty());

    tmpMGB = excessiveBlockSize - 1;

    str = MiningBlockSizeValidator(tmpMGB, NULL, true);
    BOOST_CHECK(str.empty());

    maxGeneratedBlock = 8888881;
    str = MiningBlockSizeValidator(tmpMGB, NULL, false);
    BOOST_CHECK(str.empty());

    str = MiningBlockSizeValidator(tmpMGB, (uint64_t *) 42, true);
    BOOST_CHECK(str.empty());

    tmpMGB = excessiveBlockSize + 1;

    str = MiningBlockSizeValidator(tmpMGB, NULL, true);
    BOOST_CHECK(! str.empty());

    str = MiningBlockSizeValidator(tmpMGB, NULL, false);
    BOOST_CHECK(str.empty());

    str = MiningBlockSizeValidator(tmpMGB, (uint64_t *) 42, true);
    BOOST_CHECK(! str.empty());

    maxGeneratedBlock = c_mgb;
    excessiveBlockSize = c_ebs;
}


BOOST_AUTO_TEST_SUITE_END()
