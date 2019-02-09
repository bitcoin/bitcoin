#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <boost/next_prior.hpp>
#include <primitives/transaction.h>
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
bool OrderBasedOnArrivalTime(std::vector<CTransactionRef>& blockVtx);
#endif // GRAPH_H