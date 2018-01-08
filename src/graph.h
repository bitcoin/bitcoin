#ifndef GRAPH_H
#define GRAPH_H
#include <list>
#include <stack>
#define NIL -1
using namespace std;

// A class that represents an directed graph
class Graph
{
	int V;    // No. of vertices
	list<int> *adj;    // A dynamic array of adjacency lists

					   // A Recursive DFS based function used by SCC()
	void SCCUtil(int u, int disc[], int low[],
		stack<int> *st, bool stackMember[], list<int>&);
public:
	Graph(int V);   // Constructor
	~Graph();
	void addEdge(int v, int w);   // function to add an edge to graph
	void SCC(list<int>&);    // prints strongly connected components
};


#endif // GRAPH_H