// A C++ class for topological sorting of a directed graph for syscoin service sorting on POW
#include "graph.h"
void CGraph::TopologicSort(unordered_set<string> &results)
{
	unordered_set<string> visited;
	unordered_set<string> pending;

	Visit(Nodes, results, visited, pending);
}

void CGraph::Visit(vector<CGraphNode> &graph, unordered_set<string> &results, unordered_set<string> &visited, unordered_set<string> &pending)
{
	// Foreach node in the graph
	for (auto& n : graph) {
		{
			// Skip if node has been visited
			if (!visited.count(n.Data))
			{
				if (!pending.count(n.Data))
				{
					pending.insert(n.Data);
				}
				else
				{
					LogPrintf("Cycle detected (node Data=%s\n", n.Data.c_str());
					return;
				}

				// recursively call this function for every child of the current node
				Visit(n.Children, results, visited, pending);

				if (pending.count(n.Data))
				{
					pending.erase(n.Data);
				}

				visited.insert(n.Data);

				// Made it past the recusion part, so there are no more dependents.
				// Therefore, append node to the output list.
				results.insert(n.Data);
			}
		}
	}
