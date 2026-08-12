#include <p2p/routing.h>
#include <common/args.h>
#include <util/log.h>

void p2p::InitRouting()
{
    if (gArgs.GetBoolArg("-enable-poa", false)) {
        LogInfo("P2P routing prototype initialized (feature-flag -enable-poa)");
        // TODO: implement cluster/shard-aware routing, tree overlay, etc.
    } else {
        LogInfo("P2P routing not initialized (PoA disabled)");
    }
}

void p2p::ShutdownRouting()
{
    LogInfo("P2P routing shutdown");
}
