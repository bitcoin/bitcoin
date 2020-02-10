#include "services/graph.h"
#include "services/asset.h"
#include "services/assetallocation.h"
#include "base58.h"
#include "validation.h"
using namespace boost;
using namespace std;
typedef typename std::vector<int> container;
typedef std::map<CWitnessAddress, int> AddressMap;
bool CreateGraphFromVTX(const std::vector<CTransactionRef>& blockVtx, Graph &graph, std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	AddressMap mapAddressIndex;
	int op;
	for (unsigned int n = 0; n< blockVtx.size(); n++) {
		const CTransactionRef txRef = blockVtx[n];
		if (!txRef)
			continue;
		const CTransaction &tx = *txRef;
		if (IsAssetAllocationTx(tx))
		{
            CAssetAllocation allocation(tx);
			if (!allocation.IsNull())
			{	
				AddressMap::const_iterator it;
				const CWitnessAddress &sender = allocation.assetAllocationTuple.witnessAddress;
				it = mapAddressIndex.find(sender);
	
				if (it == mapAddressIndex.end()) {
					vertices.push_back(add_vertex(graph));
					mapAddressIndex[sender] = vertices.size() - 1;
				}
				mapTxIndex[mapAddressIndex[sender]].push_back(n);
				
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const CWitnessAddress& receiver = allocationInstance.first;
						AddressMap::const_iterator it = mapAddressIndex.find(receiver);
						if (it == mapAddressIndex.end()) {
							vertices.push_back(add_vertex(graph));
							mapAddressIndex[receiver] = vertices.size() - 1;
						}
						// the graph needs to be from index to index 
						add_edge(vertices[mapAddressIndex[sender]], vertices[mapAddressIndex[receiver]], graph);
					}
				}
			}
		}
	}
	return mapTxIndex.size() > 0;
}
// remove cycles in a graph and create a DAG, modify the blockVtx passed in to remove conflicts, the conflicts should be added back to the end of this vtx after toposort
void GraphRemoveCycles(const std::vector<CTransactionRef>& blockVtx, std::vector<int> &conflictedIndexes, Graph& graph, const std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex) {
	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	for (auto& nVertex : clearedVertices) {
		IndexMap::const_iterator it = mapTxIndex.find(nVertex);
		if (it == mapTxIndex.end())
			continue;
		// remove from graph
		const int nVertexIndex = (*it).first;
		if (nVertexIndex >= (int)vertices.size())
			continue;
		boost::clear_out_edges(vertices[nVertexIndex], graph);
		const std::vector<int> &vecTx = (*it).second;
		// mapTxIndex knows of the mapping between vertices and tx vout positions
		for (auto& nIndex : vecTx) {
			if (nIndex >= (int)blockVtx.size())
				continue;
			conflictedIndexes.push_back(nIndex);
		}
		mapTxIndex.erase(it);
	}
}
bool DAGTopologicalSort(const std::vector<CTransactionRef>& blockVtx, const Graph& graph, const IndexMap &mapTxIndex) {
	container c;
	try
	{
		topological_sort(graph, std::back_inserter(c));
	}
	catch (not_a_dag const& e){
		LogPrint(BCLog::SYS, "DAGTopologicalSort: Not a DAG: %s\n", e.what());
		return false;
	}
    reverse(c.begin(), c.end());
 	for (auto& nVertex : c) {	
		// this may not find the vertex if its a receiver (we only want to process sender as tx is tied to sender)	
		IndexMap::const_iterator it = mapTxIndex.find(nVertex);	
		if (it == mapTxIndex.end())	
			continue;	
		const std::vector<int> &vecTx = (*it).second;	
		// mapTxIndex knows of the mapping between vertices and tx index positions	
		for (auto& nIndex : vecTx) {	
			if (nIndex >= (int)blockVtx.size())	
				continue;	
			
            // 
		}	
	}
	
	return true;
}