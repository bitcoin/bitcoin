#include <node/peerman_args.h>

#include <common/args.h>
#include <net_processing.h>

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options)
{
    if(auto value{argsman.GetBoolArg("-txreconciliation")}) options.reconcile_txs = *value;
}

} // namespace node

