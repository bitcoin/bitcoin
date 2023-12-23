#ifndef BITCOIN_NODE_PEERMAN_ARGS_H
#define BITCOIN_NODE_PEERMAN_ARGS_H

#include <net_processing.h>

class ArgsManager;

namespace node {
void ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options);
} // namespace node

#endif // BITCOIN_NODE_PEERMAN_ARGS_H
