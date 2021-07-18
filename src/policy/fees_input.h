// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_POLICY_FEES_INPUT_H
#define BITCOIN_POLICY_FEES_INPUT_H

#include <amount.h>
#include <fs.h>
#include <policy/fees.h>

#include <map>
#include <memory>
#include <vector>

class CAutoFile;
class CBlockPolicyEstimator;
class UniValue;
class uint256;

class FeeEstInput
{
public:
    explicit FeeEstInput(CBlockPolicyEstimator& estimator);

    /** Open fee estimator input after construction. Load fee estimation data file and open optional log file. */
    bool open(const std::string& log_filename);

    /** Drop still unconfirmed transactions and record current estimations, if the fee estimation file is present. */
    bool close();

    /** Process all the transactions that have been included in a block */
    void processBlock(unsigned int height, const AddTxsFn& add_txs);

    /** Process a transaction added the mempool or a block */
    void processTx(const uint256& hash, unsigned int height, CAmount fee, uint32_t size, bool valid);

    /** Remove a transaction from the mempool tracking stats */
    void removeTx(const uint256& hash, bool in_block);

    /** Write estimation data to a file */
    bool writeData(const fs::path& filename);

    /** Read estimation data from a file */
    bool readData(const fs::path& filename);

    /** Write incoming block and transaction events to log file. */
    bool writeLog(const std::string& filename);

    /** Read block and transaction events from log file. */
    bool readLog(const std::string& filename, const std::function<bool(UniValue&)>& filter);

private:
    CBlockPolicyEstimator& m_estimator;
    std::unique_ptr<std::basic_ostream<char>> m_log;
};

#endif /*BITCOIN_POLICY_FEES_INPUT_H */
