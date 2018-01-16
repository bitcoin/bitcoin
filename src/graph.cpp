#include "graph.h"
#include "offer.h"
#include "cert.h"
#include "alias.h"
#include "asset.h"
#include "assetallocation.h"
using namespace boost;
using namespace std;
typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS > Graph;
typedef graph_traits<Graph> Traits;
typedef typename Traits::vertex_descriptor vertex_descriptor;
typedef typename std::vector<int> container;
typedef std::map<int, vector<int> > IndexMap;
typedef std::map<string, int> AliasMap;
bool CreateGraphFromBlock(const CBlock*pblock, Graph &graph, std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	AliasMap mapAliasIndex;
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
				AliasMap::iterator it = mapAliasIndex.find(sender);
				if (it == mapAliasIndex.end()) {
					vertices.push_back(add_vertex(graph));
					mapAliasIndex[sender] = vertices.size() - 1;
				}
				mapTxIndex[mapAliasIndex[sender]].push_back(n);
				
				LogPrintf("CreateGraphFromBlock: found asset allocation from sender %s, nOut %d\n", sender, n);
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
						LogPrintf("CreateGraphFromBlock: add edge from %s(index %d) to %s(index %d)\n", sender, mapAliasIndex[sender], receiver, mapAliasIndex[receiver]);
					}
				}
			}
		}
	}
	return mapTxIndex.size() > 0;
}
bool GraphRemoveCycles(CBlock * pblock, Graph& graph, std::vector<int> &indexesToMove) {
	LogPrintf("GraphRemoveCycles\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	IndexMap mapTxIndex;

	if (!CreateGraphFromBlock(pblock, graph, vertices, mapTxIndex)) {
		return false;
	}
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
		for (auto& nOut : vecTx) {
			if (nOut >= pblock->vtx.size())
				continue;
			LogPrintf("indexesToMove %d\n", nOut);
			indexesToMove.push_back(nOut);
		}
	}
	// outputs were saved above and loop through them backwards to remove from back to front
	std::sort(indexesToMove.begin(), indexesToMove.end(), std::greater<int>());
	for (auto& nIndex : indexesToMove) {
		pblock->vtx.erase(pblock->vtx.begin() + nIndex);
	}
	return true;
}
bool DAGTopologicalSort(CBlock* pblock, const Graph &graph, const CBlock& originalBlock, const std::vector<int> &indexesToMove) {
	LogPrintf("DAGTopologicalSort\n");
	std::vector<CTransaction> newVtx;
	IndexMap mapTxIndex;

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
	for (auto& nIndex : indexesToMove) {
		LogPrintf("add indexesToMove %d to back\n", nIndex);
		newVtx.push_back(originalBlock.vtx[nIndex]);
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