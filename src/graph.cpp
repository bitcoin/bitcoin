// A C++ class for topological sorting of a directed graph for syscoin service sorting on POW
#include "graph.h"
#include <stack>
#include <queue>
Graph::Graph(int V)
{
	this->V = V;
	adj = new list<int>[V];
}
Graph::~Graph()
{
	delete[] adj;
}
void Graph::addEdge(int v, int w)
{
	adj[v].push_back(w);
}

bool Graph::isCyclicUtil(int v, bool visited[], bool *recStack)
{
	if (visited[v] == false)
	{
		// Mark the current node as visited and part of recursion stack
		visited[v] = true;
		recStack[v] = true;

		// Recur for all the vertices adjacent to this vertex
		for (auto& i : adj[v])
		{
			if (!visited[i] && isCyclicUtil(i, visited, recStack))
				return true;
			else if (recStack[i]) {
				i = NIL;
				return true;
			}
		}

	}
	recStack[v] = false;  // remove the vertex from recursion stack
	return false;
}

// Returns true if the graph contains a cycle, else false.
bool Graph::isCyclic()
{
	// Mark all the vertices as not visited and not part of recursion
	// stack
	bool *visited = new bool[V];
	bool *recStack = new bool[V];
	for (int i = 0; i < V; i++)
	{
		visited[i] = false;
		recStack[i] = false;
	}

	// Call the recursive helper function to detect cycle in different
	// DFS trees
	for (int i = 0; i < V; i++)
		if (isCyclicUtil(i, visited, recStack))
			return true;

	return false;
}
bool Graph::topologicalSort(vector<int>& result)
{
	// Create a vector to store indegrees of all
	// vertices. Initialize all indegrees as 0.
	vector<int> in_degree(V, 0);

	// Traverse adjacency lists to fill indegrees of
	// vertices.  This step takes O(V+E) time
	for (int u = 0; u<V; u++)
	{
		for (auto& i : adj[u])
			in_degree[i]++;
	}

	// Create an queue and enqueue all vertices with
	// indegree 0
	queue<int> q;
	for (int i = 0; i < V; i++)
		if (in_degree[i] == 0)
			q.push(i);

	// Initialize count of visited vertices
	int cnt = 0;


	// One by one dequeue vertices from queue and enqueue
	// adjacents if indegree of adjacent becomes 0
	while (!q.empty())
	{
		// Extract front of queue (or perform dequeue)
		// and add it to topological order
		int u = q.front();
		q.pop();
		result.push_back(u);

		// Iterate through all its neighbouring nodes
		// of dequeued node u and decrease their in-degree
		// by 1
		for (auto& i : adj[u])

			// If in-degree becomes zero, add it to queue
			if (--in_degree[i] == 0)
				q.push(i);

		cnt++;
	}

	// Check if there was a cycle
	if (cnt != V)
	{
		return false;
	}

}
