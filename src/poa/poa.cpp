#include <poa/poa.h>
#include <common/args.h>
#include <util/log.h>
#include <atomic>

static std::atomic<bool> g_poa_enabled{false};

void poa::InitPoA()
{
    if (gArgs.GetBoolArg("-enable-poa", false)) {
        g_poa_enabled.store(true);
        LogInfo("PoA/BFT prototype enabled via -enable-poa");
        // TODO: load validators, on-chain registry, start BFT rounds, timers, etc.
    } else {
        LogInfo("PoA disabled (default)");
    }
}

void poa::ShutdownPoA()
{
    if (g_poa_enabled.load()) {
        LogInfo("Shutting down PoA module");
        g_poa_enabled.store(false);
    }
}

bool poa::IsPoAEnabled()
{
    return g_poa_enabled.load();
}
