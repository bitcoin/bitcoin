#include "graph.h"
#include "offer.h"
#include "cert.h"
#include "alias.h"
#include "asset.h"
#include "assetallocation.h"
using namespace boost;
using namespace std;
typedef adjacency_list< vecS, vecS, directedS > Graph;
typedef graph_traits<Graph> Traits;
typedef typename Traits::vertex_descriptor vertex_descriptor;
typedef typename std::vector<int> container;
typedef std::map<int, vector<int> > IndexMap;
typedef std::map<string, int> AliasMap;
bool CreateGraphFromVTX(const std::vector<CTransaction>& blockVtx, Graph &graph, std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	AliasMap mapAliasIndex;
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	int op;
	int nOut;
	for (unsigned int n = 0; n< blockVtx.size(); n++) {
		const CTransaction& tx = blockVtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			if (!DecodeAliasTx(tx, op, nOut, vvchAliasArgs))
				continue;

			if (DecodeAssetAllocationTx(tx, op, nOut, vvchArgs))
			{
				const string& sender = stringFromVch(vvchAliasArgs[0]);
				AliasMap::iterator it = mapAliasIndex.find(sender);
				if (it == mapAliasIndex.end()) {
					vertices.push_back(add_vertex(graph));
					mapAliasIndex[sender] = vertices.size() - 1;
				}
				mapTxIndex[mapAliasIndex[sender]].push_back(n);
				
				LogPrintf("CreateDAGFromBlock: found asset allocation from sender %s, nOut %d\n", sender, n);
				CAssetAllocation allocation(tx);
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const string& receiver = stringFromVch(allocationInstance.first);
						AliasMap::iterator it = mapAliasIndex.find(receiver);
						if (it == mapAliasIndex.end()) {
							vertices.push_back(add_vertex(graph));
							mapAliasIndex[receiver] = vertices.size() - 1;
						}
						// the graph needs to be from index to index 
						add_edge(vertices[mapAliasIndex[sender]], vertices[mapAliasIndex[receiver]], graph);
						LogPrintf("CreateDAGFromBlock: add edge from %s(index %d) to %s(index %d)\n", sender, mapAliasIndex[sender], receiver, mapAliasIndex[receiver]);
					}
				}
			}
		}
	}
	return mapTxIndex.size() > 0;
}
bool OrderBasedOnArrivalTime(const std::vector<CTransaction>& blockVtx, std::vector<CTransaction> &orderedVtx, std::map<int, int> &mapIndexOriginalVTxToOrderedVtx) {
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	int op;
	int nOut;
	// order the arrival times in ascending order using a map
	std::multimap<int64_t, int> orderedIndexes;
	for (unsigned int n = 0; n < blockVtx.size(); n++) {
		const CTransaction& tx = blockVtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			if (!DecodeAliasTx(tx, op, nOut, vvchAliasArgs))
				continue;

			if (DecodeAssetAllocationTx(tx, op, nOut, vvchArgs))
			{
				ArrivalTimesMap arrivalTimes;
				CAssetAllocation assetallocation(tx);
				CAssetAllocationTuple assetAllocationTuple(assetallocation.vchAsset, vvchAliasArgs[0]);
				passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
				ArrivalTimesMap::iterator it = arrivalTimes.find(tx.GetHash());
				if(it != arrivalTimes.end())
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
		mapIndexOriginalVTxToOrderedVtx[orderedVtx.size() - 1] = orderedIndex.second;
		LogPrintf("OrderBasedOnArrivalTime: mapping %d to %d with time %llu\n", orderedVtx.size() - 1, orderedIndex.second, orderedIndex.first);
	}

	if (blockVtx.size() != orderedVtx.size())
	{
		LogPrintf("OrderBasedOnArrivalTime: sorted block transaction count does not match unsorted block transaction count! sorted block count %d vs unsorted block count %d\n", orderedVtx.size(), blockVtx.size());
		return false;
	}
	return true;
}
unsigned int GraphRemoveCycles(CBlock * pblock, std::unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees) {
	LogPrintf("GraphRemoveCycles\n");
	std::vector<CTransaction> newVtx;
	std::vector<CTransaction> orderedVtx;
	std::vector<vertex_descriptor> vertices;
	std::map<int, int> mapIndexOriginalVTxToOrderedVtx;
	IndexMap mapTxIndex;
	Graph graph;
	if (!OrderBasedOnArrivalTime(pblock->vtx, orderedVtx, mapIndexOriginalVTxToOrderedVtx))
		return 0;
	if (!CreateGraphFromVTX(orderedVtx, graph, vertices, mapTxIndex))
		return 0;
	
	std::vector<int> outputsToRemove;
	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	LogPrintf("Found %d circuits\n", clearedVertices.size());
	for (auto& nVertex : clearedVertices) {
		LogPrintf("trying to clear vertex %d\n", nVertex);
		IndexMap::iterator it = mapTxIndex.find(nVertex);
		if (it == mapTxIndex.end())
			continue;
		const std::vector<int> &vecTx = (*it).second;
		// mapTxIndex knows of the mapping between vertices and tx vout positions
		for (auto& nIndex : vecTx) {
			if (nIndex >= pblock->vtx.size())
				continue;
			LogPrintf("outputsToRemove %d mapIndexOriginalVTxToOrderedVtx[nIndex] %d\n", nIndex, mapIndexOriginalVTxToOrderedVtx[nIndex]);
			// mapIndexOriginalVTxToOrderedVtx is the mapping from the ordered by arrival time index to the original block index for removal below
			outputsToRemove.push_back(mapIndexOriginalVTxToOrderedVtx[nIndex]);
		}
	}
	// outputs were saved above and loop through them backwards to remove from back to front
	std::sort(outputsToRemove.begin(), outputsToRemove.end(), std::greater<int>());
	for (auto& nIndex : outputsToRemove) {
		LogPrintf("reversed outputsToRemove %d\n", nIndex);
		nFees -= pblocktemplate->vTxFees[nIndex];
		nBlockSigOps -= pblocktemplate->vTxSigOps[nIndex];
		nBlockSize -= pblock->vtx[nIndex].GetTotalSize();
		pblock->vtx.erase(pblock->vtx.begin() + nIndex);
		nBlockTx--;
		pblocktemplate->vTxFees.erase(pblocktemplate->vTxFees.begin() + nIndex);
		pblocktemplate->vTxSigOps.erase(pblocktemplate->vTxSigOps.begin() + nIndex);
	}
	return clearedVertices.size();
}
bool DAGTopologicalSort(CBlock * pblock) {
	LogPrintf("DAGTopologicalSort\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	IndexMap mapTxIndex;
	Graph graph;

	if (!CreateGraphFromVTX(pblock->vtx, graph, vertices, mapTxIndex)) {
		return true;
	}
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
	newVtx.push_back(pblock->vtx[0]);

	// add sys tx's to newVtx in reverse sorted order
	reverse(c.begin(), c.end());
	string ordered = "";
	for (auto& nVertex : c) {
		LogPrintf("add sys tx in sorted order\n");
		// this may not find the vertex if its a receiver (we only want to process sender as tx is tied to sender)
		IndexMap::iterator it = mapTxIndex.find(nVertex);
		if (it == mapTxIndex.end())
			continue;
		const std::vector<int> &vecTx = (*it).second;
		// mapTxIndex knows of the mapping between vertices and tx vout positions
		for (auto& nOut : vecTx) {
			if (nOut >= pblock->vtx.size())
				continue;
			newVtx.push_back(pblock->vtx[nOut]);
			ordered += strprintf("%d", nOut);
		}
	}
	LogPrintf("topological ordering: %s\n", ordered);
	// add non sys tx's to end of newVtx
	for (unsigned int nOut = 1; nOut< pblock->vtx.size(); nOut++) {
		const CTransaction& tx = pblock->vtx[nOut];
		if (tx.nVersion != SYSCOIN_TX_VERSION)
		{
			newVtx.push_back(pblock->vtx[nOut]);
		}
	}
	LogPrintf("newVtx size %d vs pblock.vtx size %d\n", newVtx.size(), pblock->vtx.size());
	if (pblock->vtx.size() != newVtx.size())
	{
		LogPrintf("DAGTopologicalSort: sorted block transaction count does not match unsorted block transaction count!\n");
		return false;
	}
	// set newVtx to block's vtx so block can process as normal
	pblock->vtx = newVtx;
	return true;
}