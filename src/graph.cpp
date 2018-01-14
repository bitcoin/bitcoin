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

bool CreateDAGFromBlock(const CBlock*pblock, Graph &graph, std::vector<vertex_descriptor> &vertices, std::unordered_map<int, int> &mapTxIndex) {
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
					mapTxIndex[vertices.size() - 1] = n;
				}
				
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
	std::unordered_map<int, int> mapTxIndex;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex)) {
		return true;
	}

	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	LogPrintf("Found %d circuits\n", clearedVertices.size());
	// iterate backwards over sorted list of vertices, we can do this because we remove vertices from end to beginning, 
	// which invalidate iterators from positon removed to end (we don't care about those after removal since we are iterating backwards to begining)
	for (auto& nVertex : clearedVertices) {
		if (!mapTxIndex.count(nVertex))
			continue;
		// mapTxIndex knows of the mapping between vertices and tx vout position
		const int &nOut = mapTxIndex[nVertex];
		if (nOut >= pblock->vtx.size())
			continue;
		LogPrintf("cleared vertex, erasing nOut %d\n", nOut);
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
	// set newVtx to block's vtx so block can process as normal
	pblock->vtx = newVtx;
	return true;
}