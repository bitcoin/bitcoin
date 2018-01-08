#ifndef GRAPH_H
#define GRAPH_H
#include <unordered_set>
#include <vector>
#include <string.h>
#include "util.h"
using namespace std;
struct CGraphNode
{
	string Data;
	vector<CGraphNode> Children;

	CGraphNode()
	{
		
	}
	CGraphNode(const string& _Data)
	{
		Data = _Data;
		Children.clear();
	}
};

// An range has start and end index
class CGraph {
public:
	CGraph() {
	}
	vector<CGraphNode> Nodes;
	void TopologicSort(unordered_set<string> &results);

private:
	void Visit(vector<CGraphNode> &graph, unordered_set<string> &results, unordered_set<string> &visited, unordered_set<string> &pending);

};

#endif // GRAPH_H