#include "services/graph.h"
#include "services/asset.h"
#include "services/assetallocation.h"
#include "base58.h"
#include "validation.h"
using namespace std;
extern ArrivalTimesMapImpl arrivalTimesMap;
bool OrderBasedOnArrivalTime(std::vector<CTransactionRef>& blockVtx) {
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<CTransactionRef> orderedVtx;
	int op;
	AssertLockHeld(cs_main);
	CCoinsViewCache view(pcoinsTip.get());
	// order the arrival times in ascending order using a map
	std::multimap<int64_t, int> orderedIndexes;
	for (unsigned int n = 0; n < blockVtx.size(); n++) {
		const CTransactionRef txRef = blockVtx[n];
		if (!txRef)
			continue;
		const CTransaction &tx = *txRef;
		if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET)
		{
			if (DecodeAssetAllocationTx(tx, op, vvchArgs))
			{
				LOCK(cs_assetallocationarrival);
				
				CAssetAllocation assetallocation(tx);
				ArrivalTimesMap &arrivalTimes = arrivalTimesMap[assetallocation.assetAllocationTuple.ToString()];

				ArrivalTimesMap::iterator it = arrivalTimes.find(tx.GetHash());
				if (it != arrivalTimes.end())
					orderedIndexes.insert(make_pair((*it).second, n));
				// we don't have this in our arrival times list, means it must be rejected via consensus so add it to the end
				else
					orderedIndexes.insert(make_pair(INT64_MAX, n));
				continue;
			}
		}
		// add normal tx's to orderedvtx, 
		orderedVtx.emplace_back(txRef);
	}
	for (auto& orderedIndex : orderedIndexes) {
		orderedVtx.emplace_back(blockVtx[orderedIndex.second]);
	}
	if (blockVtx.size() != orderedVtx.size())
	{
		LogPrint(BCLog::SYS, "OrderBasedOnArrivalTime: sorted block transaction count does not match unsorted block transaction count! sorted block count %d vs unsorted block count %d\n", orderedVtx.size(), blockVtx.size());
		return false;
	}
	blockVtx = orderedVtx;
	return true;
}
