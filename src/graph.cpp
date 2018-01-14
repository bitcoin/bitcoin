#include "graph.h"
#include "offer.h"
#include "cert.h"
#include "alias.h"
#include "asset.h"
#include "assetallocation.h"
using namespace boost;
typedef adjacency_list< vecS, vecS, directedS > Graph;
typedef graph_traits<Graph> Traits;
typedef typename Traits::vertex_descriptor vertex_descriptor;
typedef typename std::vector< vertex_descriptor > container;
typedef typename property_map<Graph, vertex_index_t>::const_type IndexMap;

bool CreateDAGFromBlock(const CBlock*pblock, Graph &graph, std::vector<vertex_descriptor> &vertices, std::unordered_map<int, int> &mapTxIndex, sorted_vector *vecTxIndexToRemove=NULL) {
	std::map<string, int> mapAliasIndex;
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	int op;
	int nOut;
	for (unsigned int n = 0; n< pblock->vtx.size(); n++) {
		const CTransaction& tx = pblock->vtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			if (!DecodeAliasTx(tx, op, nOut, vvchAliasArgs))
				continue;

			if (DecodeAssetAllocationTx(tx, op, nOut, vvchArgs))
			{
				const string& sender = stringFromVch(vvchAliasArgs[0]);
				if (mapAliasIndex.count(sender) == 0) {
					vertices.push_back(add_vertex(graph));
					mapAliasIndex[sender] = vertices.size() - 1;
				}
				// remove duplicate senders and avoid processing into DAG
				else if(vecTxIndexToRemove != NULL)
				{
					vecTxIndexToRemove->insert(n);
					continue;
				}
				mapTxIndex[mapAliasIndex[sender]] = n;
				
				LogPrintf("CreateDAGFromBlock: found asset allocation from sender %s, nOut %d\n", sender, n);
				CAssetAllocation allocation(tx);
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const string& receiver = stringFromVch(allocationInstance.first);
						if (mapAliasIndex.count(receiver) == 0) {
							vertices.push_back(add_vertex(graph));
							mapAliasIndex[receiver] = vertices.size() - 1;
							mapTxIndex[vertices.size() - 1] = n;
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
unsigned int DAGRemoveCycles(CBlock * pblock, std::unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees) {
	LogPrintf("DAGRemoveCycles\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	std::unordered_map<int, int> mapTxIndex;
	sorted_vector<int> vecTxIndexToRemove;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex, &vecTxIndexToRemove)) {
		return true;
	}

	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	LogPrintf("Found %d circuits\n", clearedVertices.size());
	// add vertices to vecTxIndexToRemove only if they exist in mapTxIndex (which keeps track of all the senders tx index's)
	// we only want to remove sender's tx since its one to many relationship between sender and receiver respectively, sender is the initiator of the tx
	// receivers do not even have their own tx's
	for (auto& nVertex : clearedVertices) {
		if (!mapTxIndex.count(nVertex))
			continue;
		vecTxIndexToRemove.push_back(mapTxIndex[nVertex]);
	}
	// iterate backwards over sorted list of vertices, we can do this because we remove vertices from end to beginning, 
	// which invalidate iterators from positon removed to end (we don't care about those after removal since we are iterating backwards to begining)
	reverse(vecTxIndexToRemove.begin(), vecTxIndexToRemove.end());
	for (auto& nIndex : vecTxIndexToRemove) {
		if (nIndex >= pblock->vtx.size())
			continue;
		LogPrintf("cleared vertex, erasing nIndex %d\n", nIndex);
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
	std::unordered_map<int, int> mapTxIndex;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex)) {
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

	// add sys tx's to newVtx in sorted order
	reverse(c.begin(), c.end());
	string ordered = "";
	for (auto& nVertex : c) {
		if (!mapTxIndex.count(nVertex))
			continue;
		LogPrintf("add sys tx in sorted order\n");
		const int &nOut = mapTxIndex[nVertex];
		if (nOut >= pblock->vtx.size())
			continue;
		LogPrintf("push nOut %d\n", nOut);
		newVtx.push_back(pblock->vtx[nOut]);
		ordered += strprintf("%d ", nOut);
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