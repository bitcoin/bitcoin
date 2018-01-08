#ifndef GRAPH_H
#define GRAPH_H
#include <unordered_set>
#include <string.h>
#include "util.h"
using namespace std;
struct CGraphNode
{
	string Data;
	unordered_set<CGraphNode> Children;

	CGraphNode()
	{
		
	}
};


// An range has start and end index
class CGraph {
public:
	CGraph() {
	}
	unordered_set<CGraphNode> Nodes;
	void TopologicSort(unordered_set<string> &results);

private:
	void Visit(unordered_set<CGraphNode> &graph, unordered_set<string> &results, unordered_set<string> &visited, unordered_set<string> &pending);

};

#endif // GRAPH_H