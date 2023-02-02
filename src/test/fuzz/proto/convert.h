#ifndef BITCOIN_TEST_FUZZ_PROTO_CONVERT_H
#define BITCOIN_TEST_FUZZ_PROTO_CONVERT_H

#include <primitives/transaction.h>
#include <txmempool.h>

#include <test/fuzz/proto/mempool_options.pb.h>
#include <test/fuzz/proto/transaction.pb.h>

CTransactionRef ConvertTransaction(const common_fuzz::Transaction& proto_tx);

CTxMemPool::Options ConvertMempoolOptions(const common_fuzz::MempoolOptions& mempool_opts);

#endif
