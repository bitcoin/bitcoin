#ifndef GRAPH_H
#define GRAPH_H
#include <list>
#include <vector>
#define NIL -1
using namespace std;

// A class that represents an directed graph
class Graph
{
	int V;    // No. of vertices
	list<int> *adj;    // A dynamic array of adjacency lists

					   // A Recursive DFS based function used by SCC()
	bool isCyclicUtil(int v, bool visited[], bool *recStack);
public:
	Graph(int V);   // Constructor
	~Graph();
	void addEdge(int v, int w);   // function to add an edge to graph
	bool topologicalSort(vector<int>& result);
	bool isCyclic();
	
};


#endif // GRAPH_H