#include <validation.h>
#include <services/graph.h>
#include <services/asset.h>
#include <services/assetallocation.h>
using namespace std;
extern ArrivalTimesMapImpl arrivalTimesMap;
extern CCriticalSection cs_assetallocationarrival;
bool OrderBasedOnArrivals(std::vector<CTransactionRef>& blockVtx) {
	std::vector<CTransactionRef> orderedVtx;
	AssertLockHeld(cs_main);
    AssertLockHeld(mempool.cs);
	// order the arrival times in ascending order using a map
	std::multimap<int64_t, int> orderedAssetIndexes;
    std::map<uint256, int> mapTXIDToOutputNumber;
    std::set<uint256> setOrderedAssets;
    // track output numbers linked to transaction so during mempool dependency lookup we can add output number to the new order below see setAncestors
    for (unsigned int n = 0; n < blockVtx.size(); n++) {
        mapTXIDToOutputNumber[blockVtx[n].get()->GetHash()] = n;
    }
	for (unsigned int n = 0; n < blockVtx.size(); n++) {
		const CTransactionRef &txRef = blockVtx[n];
        const uint256& txHash = txRef->GetHash();
        if(setOrderedAssets.find(txHash) != setOrderedAssets.end())
            continue;
		if (IsAssetAllocationTx(txRef->nVersion))
		{
			LOCK(cs_assetallocationarrival);
			
            CAssetAllocation theAssetAllocation(*txRef);
            ActorSet actorSet;
            GetActorsFromAssetAllocationTx(theAssetAllocation, txRef->nVersion, true, false, actorSet);
            
            ArrivalTimesMap &arrivalTimes = arrivalTimesMap[*actorSet.begin()];
            //  if we have arrival time for this transaction we want to add it ordered by the time it was received
            auto it = std::find_if( arrivalTimes.begin(), arrivalTimes.end(),
                [&txHash](const std::pair<uint256, std::pair<uint32_t, int64_t> >& element){ return element.first == txHash;} );
			if (it != arrivalTimes.end()){
                CTxMemPool::setEntries setAncestors;
                uint64_t noLimit = std::numeric_limits<uint64_t>::max();
                std::string dummy;
                // get any mempool ancestors that this transaction depends on and add it directly proceeding the transaction
                CTxMemPoolEntry entry = *mempool.mapTx.find(txHash);
                mempool.CalculateMemPoolAncestors(entry, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);
                for (CTxMemPool::txiter itAncestor : setAncestors) {
                    const uint256& txHashAncestor = itAncestor->GetSharedTx()->GetHash();
                    // if it was already added to the ordered list through non-ancestor transaction then skip we don't want to add it again
                    if(setOrderedAssets.find(txHashAncestor) != setOrderedAssets.end())
                         continue;
                    // if this transaction is already part of the block then add the output number based on the lookup
                    auto itOutputNumber = mapTXIDToOutputNumber.find(txHashAncestor);
                    if(itOutputNumber != mapTXIDToOutputNumber.end()){
                        // ordering in a multiset should be preserved in insertion order for duplicate keys starting with c++11
                        orderedAssetIndexes.insert(make_pair((*it).second.second, itOutputNumber->second));
                        // store the fact that this transaction is already stored so when we try to add non asset transactions to this block it will cross-reference this and not potentially add it multiple times
                        setOrderedAssets.insert(txHashAncestor);
                    }                   
                }
                // add non-ancestor transaction to the ordered lists
                orderedAssetIndexes.insert(make_pair((*it).second.second, n));
                setOrderedAssets.insert(txHash);
            }
			// we don't have this in our arrival times list, means it must be rejected via consensus so add it to the end
			else{
				orderedAssetIndexes.insert(make_pair(INT64_MAX, n));
                setOrderedAssets.insert(txHash);
            }
			continue;	
		}
	}
    // walk through block and add transactions to the block that are not involved in assets
    for (unsigned int n = 0; n < blockVtx.size(); n++) {
		const CTransactionRef &txRef = blockVtx[n];
        const uint256& txHash = txRef->GetHash();
        // ensure that dependent transactions added via setAncestors is not added again
        if(setOrderedAssets.find(txHash) == setOrderedAssets.end())
            orderedVtx.emplace_back(blockVtx[n]);
    }
    // add the asset transactions and their ancestors to the end of the block
	for (auto& orderedIndex : orderedAssetIndexes) {
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