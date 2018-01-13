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

bool CreateDAGFromBlock(const CBlock*pblock, Graph &graph, std::vector<vertex_descriptor> &vertices, std::unordered_map<int, vector<int> > &mapTxIndex) {
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
				mapTxIndex[mapAliasIndex[sender]].push_back(n);
				LogPrintf("CreateDAGFromBlock: found asset allocation from sender %s, nOut %d\n", sender, n);
				CAssetAllocation allocation(tx);
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const string& receiver = stringFromVch(allocationInstance.first);
						if (mapAliasIndex.count(receiver) == 0) {
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
unsigned int DAGRemoveCycles(CBlock * pblock, std::unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees) {
	LogPrintf("DAGRemoveCycles\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	std::unordered_map<int, vector<int> > mapTxIndex;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex)) {
		return true;
	}

	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	// keep track of outputs to remove in order
	sorted_vector<int> outputsToRemove;
	hawick_circuits(graph, visitor);
	LogPrintf("Found %d circuits\n", clearedVertices.size());
	for (auto& nVertex : clearedVertices) {
		LogPrintf("trying to clear vertex %d\n", nVertex);
		// performance trick on map to avoid a O(N) search miss time, we know that if the nVertex is within the bounds of map it exists
		if (mapTxIndex.size() > nVertex) {
			// mapTxIndex knows of the mapping between vertices and tx vout positions, this is O(1) for unordered_map lookup
			for (auto& nOut : mapTxIndex[nVertex]) {
				if (nOut >= pblock->vtx.size())
					continue;
				LogPrintf("outputsToRemove %d\n", nOut);
				outputsToRemove.insert(nOut);
			}
		}
	}
	// outputs were saved above and loop through them backwards to remove from back to front
	reverse(outputsToRemove.begin(), outputsToRemove.end());
	for (auto& nOut : outputsToRemove) {
		LogPrintf("reversed outputsToRemove %d\n", nOut);
		nFees -= pblocktemplate->vTxFees[nOut];
		nBlockSigOps -= pblocktemplate->vTxSigOps[nOut];
		nBlockSize -= pblock->vtx[nOut].GetTotalSize();
		pblock->vtx.erase(pblock->vtx.begin() + nOut);
		nBlockTx--;
		pblocktemplate->vTxFees.erase(pblocktemplate->vTxFees.begin() + nOut);
		pblocktemplate->vTxSigOps.erase(pblocktemplate->vTxSigOps.begin() + nOut);
	}
	return clearedVertices.size();
}
bool DAGTopologicalSort(CBlock * pblock) {
	LogPrintf("DAGTopologicalSort\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	std::unordered_map<int, vector<int> > mapTxIndex;
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
	const IndexMap &indices = get(vertex_index, (const Graph &)graph);
	// add coinbase
	newVtx.push_back(pblock->vtx[0]);

	// add sys tx's to newVtx in sorted order
	reverse(c.begin(), c.end());
	string ordered = "";
	for (auto& nVertex :c) {
		LogPrintf("add sys tx in sorted order\n");
		// performance trick on map to avoid a O(N) search miss time, we know that if the nVertex is within the bounds of map it exists
		if (mapTxIndex.size() > nVertex) {
			// mapTxIndex knows of the mapping between vertices and tx vout positions, this is O(1) for unordered_map lookup
			for (auto& nOut : mapTxIndex[nVertex]) {
				if (nOut >= pblock->vtx.size())
					continue;
				LogPrintf("push nOut %d\n", nOut);
				newVtx.push_back(pblock->vtx[nOut]);
				ordered += strprintf("%d ", nOut);
			}
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
	// set newVtx to block's vtx so block can process as normal
	pblock->vtx = newVtx;
	return true;
}