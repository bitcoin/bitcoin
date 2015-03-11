#ifndef BITCOIN_TEST_TEST_BITCOIN_H
#define BITCOIN_TEST_TEST_BITCOIN_H

#include "key.h"
#include "txdb.h"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <vector>

//
// Testing fixture for unit tests that need
// a UNITTEST block chain.
//
struct TestingSetup {
public:
    TestingSetup();
    TestingSetup(CBaseChainParams::Network network);
    ~TestingSetup();

protected:
    void Init(CBaseChainParams::Network network);

private:
    CCoinsViewDB *pcoinsdbview;
    boost::filesystem::path pathTemp;
    boost::thread_group threadGroup;
};

class CBlock;
struct CMutableTransaction;
class CScript;

//
// Testing fixture that pre-creates a
// 100-block REGTEST-mode block chain
//
struct TestChain100Setup : public TestingSetup {
    TestChain100Setup();

    // Create a new block with just given transactions, coinbase paying to
    // scriptPubKey, and try to add it to the current chain.
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns,
                                 const CScript& scriptPubKey);

    ~TestChain100Setup();

    std::vector<CTransaction> coinbaseTxns; // For convenience, coinbase transactions
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

#endif
