#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/hawick_circuits.hpp>
#include <boost/next_prior.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <unordered_map>
#include <vector>
#include "miner.h"
typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS > Graph;
typedef boost::graph_traits<Graph> Traits;
typedef typename Traits::vertex_descriptor vertex_descriptor;
typedef std::map<int, std::vector<int> > IndexMap;
typedef std::map<std::string, int> AliasMap;
template <class T, class Compare = std::less<T> >
struct sorted_vector {
	std::vector<T> V;
	Compare cmp;
	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;
	iterator begin() { return V.begin(); }
	iterator end() { return V.end(); }
	const_iterator begin() const { return V.begin(); }
	const_iterator end() const { return V.end(); }
	size_t size() const { return V.size(); }
	sorted_vector(const Compare& c = Compare())
		: V(), cmp(c) {}
	template <class InputIterator>
	sorted_vector(InputIterator first, InputIterator last,
		const Compare& c = Compare())
		: V(first, last), cmp(c)
	{
		std::sort(begin(), end(), cmp);
	}
	
	iterator insert(const T& t) {
		iterator i = lower_bound(begin(), end(), t, cmp);
		if (i == end() || cmp(t, *i))
			V.insert(i, t);
		return i;
	}
	const_iterator find(const T& t) const {
		const_iterator i = lower_bound(begin(), end(), t, cmp);
		return i == end() || cmp(t, *i) ? end() : i;
	}
};
template<typename T>
typename std::vector<T>::iterator
const_iterator_cast(std::vector<T>& v, typename std::vector<T>::const_iterator iter)
{
	return v.begin() + (iter - v.cbegin());
}
template <typename ClearedVertices>
struct cycle_visitor
{
	cycle_visitor(ClearedVertices& vertices)
		: cleared(vertices)
	{}

	template <typename Path, typename Graph>
	void cycle(Path const& p, Graph & g)
	{
		if (p.empty())
			return;
		const int &nValue = *(boost::prior(p.end()));
		cleared.insert(nValue);
	}
	ClearedVertices& cleared;
};
bool OrderBasedOnArrivalTime(const int &nHeight,std::vector<CTransactionRef>& blockVtx);
bool CreateGraphFromVTX(const int &nHeight, const std::vector<CTransactionRef>& blockVtx, Graph &graph, std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex);
void GraphRemoveCycles(const std::vector<CTransactionRef>& blockVtx, std::vector<int> &conflictedIndexes, Graph& graph, const std::vector<vertex_descriptor> &vertices, IndexMap &mapTxIndex);
bool DAGTopologicalSort(std::vector<CTransactionRef>& blockVtx, const std::vector<int> &conflictedIndexes, const Graph& graph, const IndexMap &mapTxIndex);
#endif // GRAPH_H