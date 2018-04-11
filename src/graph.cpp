#include "graph.h"
#include "offer.h"
#include "cert.h"
#include "alias.h"
#include "asset.h"
#include "assetallocation.h"
using namespace boost;
using namespace std;
typedef typename std::vector<int> container;
bool OrderBasedOnArrivalTime(std::vector<CTransaction>& blockVtx) {
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	std::vector<CTransaction> orderedVtx;
	int op;
	// order the arrival times in ascending order using a map
	std::multimap<int64_t, int> orderedIndexes;
	for (unsigned int n = 0; n < blockVtx.size(); n++) {
		const CTransaction& tx = blockVtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			if (DecodeAssetAllocationTx(tx, op, vvchArgs))
			{
				if (!FindAliasInTx(tx, vvchAliasArgs)) {
					continue;
				}
				ArrivalTimesMap arrivalTimes;
				CAssetAllocation assetallocation(tx);
				CAssetAllocationTuple assetAllocationTuple(assetallocation.vchAsset, vvchAliasArgs[0]);
				passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
				ArrivalTimesMap::iterator it = arrivalTimes.find(tx.GetHash());
				if (it != arrivalTimes.end())
					orderedIndexes.insert(make_pair((*it).second, n));
				// we don't have this in our arrival times list, means it must be rejected via consensus so add it to the end
				else
					orderedIndexes.insert(make_pair(INT64_MAX, n));
				continue;
			}
			else if (DecodeOfferTx(tx, op, vvchArgs))
			{
				ArrivalTimesMap arrivalTimes;
				COffer offer(tx);
				pofferdb->ReadISArrivalTimes(offer.vchOffer, arrivalTimes);
				ArrivalTimesMap::iterator it = arrivalTimes.find(tx.GetHash());
				if (it != arrivalTimes.end())
					orderedIndexes.insert(make_pair((*it).second, n));
				// we don't have this in our arrival times list, means it must be rejected via consensus so add it to the end
				else
					orderedIndexes.insert(make_pair(INT64_MAX, n));
				continue;
			}
			else if (DecodeCertTx(tx, op, vvchArgs))
			{
				ArrivalTimesMap arrivalTimes;
				CCert cert(tx);
				pcertdb->ReadISArrivalTimes(cert.vchCert, arrivalTimes);
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
		orderedVtx.push_back(tx);
	}
	for (auto& orderedIndex : orderedIndexes) {
		orderedVtx.push_back(blockVtx[orderedIndex.second]);
	}
	if (blockVtx.size() != orderedVtx.size())
	{
		LogPrintf("OrderBasedOnArrivalTime: sorted block transaction count does not match unsorted block transaction count! sorted block count %d vs unsorted block count %d\n", orderedVtx.size(), blockVtx.size());
		return false;
	}
	blockVtx = orderedVtx;
	return true;
}
bool CreateGraphFromVTX(const std::vector<CTransaction>& blockVtx, Graph &graph, std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	AliasMap mapAliasIndex;
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	int op;
	for (unsigned int n = 0; n< blockVtx.size(); n++) {
		const CTransaction& tx = blockVtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			{
				LOCK(cs_main);
				if (!FindAliasInTx(tx, vvchAliasArgs)) {
					continue;
				}
			}

			if (DecodeAssetAllocationTx(tx, op, vvchArgs))
			{
				const string& sender = stringFromVch(vvchAliasArgs[0]);
				AliasMap::const_iterator it = mapAliasIndex.find(sender);
				if (it == mapAliasIndex.end()) {
					vertices.push_back(add_vertex(graph));
					mapAliasIndex[sender] = vertices.size() - 1;
				}
				mapTxIndex[mapAliasIndex[sender]].push_back(n);
				CAssetAllocation allocation(tx);
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const string& receiver = stringFromVch(allocationInstance.first);
						AliasMap::const_iterator it = mapAliasIndex.find(receiver);
						if (it == mapAliasIndex.end()) {
							vertices.push_back(add_vertex(graph));
							mapAliasIndex[receiver] = vertices.size() - 1;
						}
						// the graph needs to be from index to index 
						add_edge(vertices[mapAliasIndex[sender]], vertices[mapAliasIndex[receiver]], graph);
					}
				}
				else if (!allocation.listSendingAllocationInputs.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationInputs) {
						const string& receiver = stringFromVch(allocationInstance.first);
						AliasMap::const_iterator it = mapAliasIndex.find(receiver);
						if (it == mapAliasIndex.end()) {
							vertices.push_back(add_vertex(graph));
							mapAliasIndex[receiver] = vertices.size() - 1;
						}
						// the graph needs to be from index to index 
						add_edge(vertices[mapAliasIndex[sender]], vertices[mapAliasIndex[receiver]], graph);
					}
				}
			}
		}
	}
	return mapTxIndex.size() > 0;
}
// remove cycles in a graph and create a DAG, modify the blockVtx passed in to remove conflicts, the conflicts should be added back to the end of this vtx after toposort
void GraphRemoveCycles(const std::vector<CTransaction>& blockVtx, std::vector<int> &conflictedIndexes, Graph& graph, const std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	std::vector<CTransaction> newVtx;
	std::vector<CTransaction> orderedVtx;
	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	for (auto& nVertex : clearedVertices) {
		IndexMap::const_iterator it = mapTxIndex.find(nVertex);
		if (it == mapTxIndex.end())
			continue;
		// remove from graph
		const int nVertexIndex = (*it).first;
		if (nVertexIndex >= vertices.size())
			continue;
		boost::clear_out_edges(vertices[nVertexIndex], graph);
		const std::vector<int> &vecTx = (*it).second;
		// mapTxIndex knows of the mapping between vertices and tx vout positions
		for (auto& nIndex : vecTx) {
			if (nIndex >= blockVtx.size())
				continue;
			conflictedIndexes.push_back(nIndex);
		}
		mapTxIndex.erase(it);
	}
	// block gives us the transactions in order by time so we want to ensure we preserve it
	std::sort(conflictedIndexes.begin(), conflictedIndexes.end());
}
bool DAGTopologicalSort(std::vector<CTransaction>& blockVtx, const std::vector<int> &conflictedIndexes, const Graph& graph, const IndexMap &mapTxIndex) {
	std::vector<CTransaction> newVtx;
	container c;
	try
	{
		topological_sort(graph, std::back_inserter(c));
	}
	catch (not_a_dag const& e){
		LogPrintf("DAGTopologicalSort: Not a DAG: %s\n", e.what());
		return false;
	}
	// add coinbase
	newVtx.push_back(blockVtx[0]);

	// add sys tx's to newVtx in reverse sorted order
	reverse(c.begin(), c.end());
	for (auto& nVertex : c) {
		// this may not find the vertex if its a receiver (we only want to process sender as tx is tied to sender)
		IndexMap::const_iterator it = mapTxIndex.find(nVertex);
		if (it == mapTxIndex.end())
			continue;
		const std::vector<int> &vecTx = (*it).second;
		// mapTxIndex knows of the mapping between vertices and tx index positions
		for (auto& nIndex : vecTx) {
			if (nIndex >= blockVtx.size())
				continue;
			newVtx.push_back(blockVtx[nIndex]);
		}
	}

	// add conflicting indexes next (should already be in order)
	for (auto& nIndex : conflictedIndexes) {
		if (nIndex >= blockVtx.size())
			continue;
		newVtx.push_back(blockVtx[nIndex]);
	}
	
	// add non-sys and other sys tx's to end of newVtx
	std::vector<vector<unsigned char> > vvchArgs;
	int op;
	for (unsigned int vOut = 1; vOut< blockVtx.size(); vOut++) {
		const CTransaction& tx = blockVtx[vOut];
		if (!DecodeAssetAllocationTx(tx, op, vvchArgs))
		{
			newVtx.push_back(blockVtx[vOut]);
		}
	}
	if (blockVtx.size() != newVtx.size())
	{
		LogPrintf("DAGTopologicalSort: sorted block transaction count does not match unsorted block transaction count!\n");
		return false;
	}
	// set newVtx to block's vtx so block can process as normal
	blockVtx = newVtx;
	return true;
}