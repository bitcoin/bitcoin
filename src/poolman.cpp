
#include <algorithm>
#include <vector>
#include "poolman.h"
#include "main.h"
#include "util.h"
#include "amount.h"
#include "utiltime.h"

using namespace std;

static int64_t PoolmanLowWater, PoolmanHighWater;
static bool janitorInit = false;
static bool floatingFee = false;
static CFeeRate origTxFee;

void TxMempoolJanitor()
{
    int64_t nStart = GetTimeMillis();

    // remember original minimum tx fee
    if (!janitorInit) {
        janitorInit = true;
        origTxFee = ::minRelayTxFee;
    }

    // query mempool stats
    unsigned long totalTx = mempool.size();
    int64_t totalSize = mempool.GetTotalTxSize();

    // if below low water, disable janitor
    if (totalSize < PoolmanLowWater || totalTx < 2) {
        if (floatingFee) {
            floatingFee = false;
            ::minRelayTxFee = origTxFee;
        }
        LogPrint("mempool", "mempool level low, run complete,  %dms\n",
                 GetTimeMillis() - nStart);
        return;
    }

    // if below high water, no more work.  floating fee might be active.
    if ((int64_t)mempool.GetTotalTxSize() < PoolmanHighWater) {
        LogPrint("mempool", "mempool level %s, run complete,  %dms\n",
                 floatingFee ? "draining" : "adequate",
                 GetTimeMillis() - nStart);
        return;
    }

    // we are above high water.  float relay fee in response.
    floatingFee = true;

    // get sorted list of fees in mempool
    vector<CFeeRate> vfees;
    mempool.queryFees(vfees);
    std::sort(vfees.begin(), vfees.end());
    std::reverse(vfees.begin(), vfees.end());
    vfees.resize(vfees.size() / 2);

    // set relay fee to lowest rate of top half of mempool
    ::minRelayTxFee = vfees.back();

    LogPrint("mempool", "mempool janitor run complete, fee rate %s, %dms\n",
             ::minRelayTxFee.ToString(),
             GetTimeMillis() - nStart);
}

void InitTxMempoolJanitor(CScheduler& scheduler)
{
    // mempool janitor execution interval.  default interval: 10 min
    int janitorInterval = GetArg("-janitorinterval", (60 * 10));

    // mempool janitor low water, high water marks
    PoolmanHighWater = GetArg("-mempoolhighwater", 0);
    if (PoolmanHighWater <= 0)
        PoolmanLowWater = -1;
    else {
        PoolmanLowWater = GetArg("-mempoollowwater", -1);
        if (PoolmanLowWater < 0 || PoolmanLowWater > PoolmanHighWater)
            PoolmanLowWater = PoolmanHighWater / 2;
    }

    // if interval too small or zero, janitor is disabled
    static const int janitorSaneInterval = 10;
    if ((janitorInterval < janitorSaneInterval) ||
        (PoolmanHighWater <= 0)) {
        janitorInterval = 0;
        PoolmanLowWater = -1;
        PoolmanHighWater = 0;
    }

    // start mempool janitor
    if (janitorInterval > 0)
        scheduler.scheduleEvery(&TxMempoolJanitor, janitorInterval * 1000);
}

