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
using namespace std;
template <class T, class Compare = std::less<T> >
struct sorted_vector {
	vector<T> V;
	Compare cmp;
	typedef typename vector<T>::iterator iterator;
	typedef typename vector<T>::const_iterator const_iterator;
	iterator begin() { return V.begin(); }
	iterator end() { return V.end(); }
	const_iterator begin() const { return V.begin(); }
	const_iterator end() const { return V.end(); }
	const size_t size() { return V.size(); }
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
unsigned int DAGRemoveCycles(CBlock * pblock, unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees);
bool DAGTopologicalSort(CBlock * pblock);
#endif // GRAPH_H