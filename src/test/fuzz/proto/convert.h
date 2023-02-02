#ifndef BITCOIN_TEST_FUZZ_PROTO_CONVERT_H
#define BITCOIN_TEST_FUZZ_PROTO_CONVERT_H

#include <net.h>
#include <node/connection_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <txmempool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/proto/block.pb.h>
#include <test/fuzz/proto/mempool_options.pb.h>
#include <test/fuzz/proto/p2p.pb.h>
#include <test/fuzz/proto/transaction.pb.h>
#include <test/fuzz/util/net.h>

CTransactionRef ConvertTransaction(const common_fuzz::Transaction& proto_tx);

CTxMemPool::Options ConvertMempoolOptions(const common_fuzz::MempoolOptions& mempool_opts);

ConnectionType ConvertConnType(const common_fuzz::ConnectionType& type);
NetPermissionFlags ConvertPermFlags(const common_fuzz::NetPermissionFlags& flags);
CNode& ConvertConnection(const common_fuzz::Connection& connection, FuzzedDataProvider& sock_data_provider);
ServiceFlags ConvertServiceFlags(const common_fuzz::ServiceFlags& flags);
std::tuple<std::string, std::vector<uint8_t>> ConvertHandshakeMsg(const common_fuzz::HandshakeMsg& msg);

CBlockHeader ConvertHeader(const common_fuzz::BlockHeader& proto_header);
std::shared_ptr<CBlock> ConvertBlock(const common_fuzz::Block& block);

#endif
