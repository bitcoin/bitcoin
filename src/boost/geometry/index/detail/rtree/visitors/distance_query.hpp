// Boost.Geometry Index
//
// R-tree distance (knn, path, etc. ) query visitor implementation
//
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2021.
// Modifications copyright (c) 2019-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DISTANCE_QUERY_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DISTANCE_QUERY_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {


struct pair_first_less
{
    template <typename First, typename Second>
    inline bool operator()(std::pair<First, Second> const& p1,
                           std::pair<First, Second> const& p2) const
    {
        return p1.first < p2.first;
    }
};

struct pair_first_greater
{
    template <typename First, typename Second>
    inline bool operator()(std::pair<First, Second> const& p1,
                           std::pair<First, Second> const& p2) const
    {
        return p1.first > p2.first;
    }
};


template <typename Value, typename Translator, typename DistanceType, typename OutIt>
class distance_query_result
{
public:
    typedef DistanceType distance_type;

    inline explicit distance_query_result(size_t k, OutIt out_it)
        : m_count(k), m_out_it(out_it)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_count, "Number of neighbors should be greater than 0");

        m_neighbors.reserve(m_count);
    }

    inline void store(Value const& val, distance_type const& curr_comp_dist)
    {
        if ( m_neighbors.size() < m_count )
        {
            m_neighbors.push_back(std::make_pair(curr_comp_dist, val));

            if ( m_neighbors.size() == m_count )
                std::make_heap(m_neighbors.begin(), m_neighbors.end(), pair_first_less());
        }
        else
        {
            if ( curr_comp_dist < m_neighbors.front().first )
            {
                std::pop_heap(m_neighbors.begin(), m_neighbors.end(), pair_first_less());
                m_neighbors.back().first = curr_comp_dist;
                m_neighbors.back().second = val;
                std::push_heap(m_neighbors.begin(), m_neighbors.end(), pair_first_less());
            }
        }
    }

    inline bool has_enough_neighbors() const
    {
        return m_count <= m_neighbors.size();
    }

    inline distance_type greatest_comparable_distance() const
    {
        // greatest distance is in the first neighbor only
        // if there is at least m_count values found
        // this is just for safety reasons since is_comparable_distance_valid() is checked earlier
        // TODO - may be replaced by ASSERT
        return m_neighbors.size() < m_count
            ? (std::numeric_limits<distance_type>::max)()
            : m_neighbors.front().first;
    }

    inline size_t finish()
    {
        typedef typename std::vector< std::pair<distance_type, Value> >::const_iterator neighbors_iterator;
        for ( neighbors_iterator it = m_neighbors.begin() ; it != m_neighbors.end() ; ++it, ++m_out_it )
            *m_out_it = it->second;

        return m_neighbors.size();
    }

private:
    size_t m_count;
    OutIt m_out_it;

    std::vector< std::pair<distance_type, Value> > m_neighbors;
};

template
<
    typename MembersHolder,
    typename Predicates,
    std::size_t DistancePredicateIndex,
    typename OutIter
>
class distance_query
    : public MembersHolder::visitor_const
{
public:
    typedef typename MembersHolder::value_type value_type;
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename index::detail::strategy_type<parameters_type>::type strategy_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef index::detail::predicates_element<DistancePredicateIndex, Predicates> nearest_predicate_access;
    typedef typename nearest_predicate_access::type nearest_predicate_type;
    typedef typename indexable_type<translator_type>::type indexable_type;

    typedef index::detail::calculate_distance<nearest_predicate_type, indexable_type, strategy_type, value_tag> calculate_value_distance;
    typedef index::detail::calculate_distance<nearest_predicate_type, box_type, strategy_type, bounds_tag> calculate_node_distance;
    typedef typename calculate_value_distance::result_type value_distance_type;
    typedef typename calculate_node_distance::result_type node_distance_type;

    static const std::size_t predicates_len = index::detail::predicates_length<Predicates>::value;

    inline distance_query(parameters_type const& parameters, translator_type const& translator, Predicates const& pred, OutIter out_it)
        : m_parameters(parameters), m_translator(translator)
        , m_pred(pred)
        , m_result(nearest_predicate_access::get(m_pred).count, out_it)
        , m_strategy(index::detail::get_strategy(parameters))
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;

        // array of active nodes
        typedef typename index::detail::rtree::container_from_elements_type<
            elements_type,
            std::pair<node_distance_type, typename allocators_type::node_pointer>
        >::type active_branch_list_type;

        active_branch_list_type active_branch_list;
        active_branch_list.reserve(m_parameters.get_max_elements());
        
        elements_type const& elements = rtree::elements(n);

        // fill array of nodes meeting predicates
        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            // if current node meets predicates
            // 0 - dummy value
            if ( index::detail::predicates_check
                    <
                        index::detail::bounds_tag, 0, predicates_len
                    >(m_pred, 0, it->first, m_strategy) )
            {
                // calculate node's distance(s) for distance predicate
                node_distance_type node_distance;
                // if distance isn't ok - move to the next node
                if ( !calculate_node_distance::apply(predicate(), it->first,
                                                     m_strategy, node_distance) )
                {
                    continue;
                }

                // if current node is further than found neighbors - don't analyze it
                if ( m_result.has_enough_neighbors() &&
                     is_node_prunable(m_result.greatest_comparable_distance(), node_distance) )
                {
                    continue;
                }

                // add current node's data into the list
                active_branch_list.push_back( std::make_pair(node_distance, it->second) );
            }
        }

        // if there aren't any nodes in ABL - return
        if ( active_branch_list.empty() )
            return;
        
        // sort array
        std::sort(active_branch_list.begin(), active_branch_list.end(), pair_first_less());

        // recursively visit nodes
        for ( typename active_branch_list_type::const_iterator it = active_branch_list.begin();
              it != active_branch_list.end() ; ++it )
        {
            // if current node is further than furthest neighbor, the rest of nodes also will be further
            if ( m_result.has_enough_neighbors() &&
                 is_node_prunable(m_result.greatest_comparable_distance(), it->first) )
                break;

            rtree::apply_visitor(*this, *(it->second));
        }

        // ALTERNATIVE VERSION - use heap instead of sorted container
        // It seems to be faster for greater MaxElements and slower otherwise
        // CONSIDER: using one global container/heap for active branches
        //           instead of a sorted container per level
        //           This would also change the way how branches are traversed!
        //           The same may be applied to the iterative version which btw suffers
        //           from the copying of the whole containers on resize of the ABLs container

        //// make a heap
        //std::make_heap(active_branch_list.begin(), active_branch_list.end(), pair_first_greater());

        //// recursively visit nodes
        //while ( !active_branch_list.empty() )
        //{
        //    //if current node is further than furthest neighbor, the rest of nodes also will be further
        //    if ( m_result.has_enough_neighbors()
        //      && is_node_prunable(m_result.greatest_comparable_distance(), active_branch_list.front().first) )
        //    {
        //        break;
        //    }

        //    rtree::apply_visitor(*this, *(active_branch_list.front().second));

        //    std::pop_heap(active_branch_list.begin(), active_branch_list.end(), pair_first_greater());
        //    active_branch_list.pop_back();
        //}
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);
        
        // search leaf for closest value meeting predicates
        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            // if value meets predicates
            if ( index::detail::predicates_check
                    <
                        index::detail::value_tag, 0, predicates_len
                    >(m_pred, *it, m_translator(*it), m_strategy) )
            {
                // calculate values distance for distance predicate
                value_distance_type value_distance;
                // if distance is ok
                if ( calculate_value_distance::apply(predicate(), m_translator(*it),
                                                     m_strategy, value_distance) )
                {
                    // store value
                    m_result.store(*it, value_distance);
                }
            }
        }
    }

    inline size_t finish()
    {
        return m_result.finish();
    }

private:
    template <typename Distance>
    static inline bool is_node_prunable(Distance const& greatest_dist, node_distance_type const& d)
    {
        return greatest_dist <= d;
    }

    nearest_predicate_type const& predicate() const
    {
        return nearest_predicate_access::get(m_pred);
    }

    parameters_type const& m_parameters;
    translator_type const& m_translator;

    Predicates m_pred;
    distance_query_result<value_type, translator_type, value_distance_type, OutIter> m_result;

    strategy_type m_strategy;
};

template <
    typename MembersHolder,
    typename Predicates,
    std::size_t DistancePredicateIndex
>
class distance_query_incremental
    : public MembersHolder::visitor_const
{
public:
    typedef typename MembersHolder::value_type value_type;
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename index::detail::strategy_type<parameters_type>::type strategy_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef index::detail::predicates_element<DistancePredicateIndex, Predicates> nearest_predicate_access;
    typedef typename nearest_predicate_access::type nearest_predicate_type;
    typedef typename indexable_type<translator_type>::type indexable_type;
    
    typedef index::detail::calculate_distance<nearest_predicate_type, indexable_type, strategy_type, value_tag> calculate_value_distance;
    typedef index::detail::calculate_distance<nearest_predicate_type, box_type, strategy_type, bounds_tag> calculate_node_distance;
    typedef typename calculate_value_distance::result_type value_distance_type;
    typedef typename calculate_node_distance::result_type node_distance_type;

    typedef typename allocators_type::size_type size_type;
    typedef typename allocators_type::const_reference const_reference;
    typedef typename allocators_type::node_pointer node_pointer;

    static const std::size_t predicates_len = index::detail::predicates_length<Predicates>::value;

    typedef typename rtree::elements_type<internal_node>::type internal_elements;
    typedef typename internal_elements::const_iterator internal_iterator;
    typedef typename rtree::elements_type<leaf>::type leaf_elements;

    typedef std::pair<node_distance_type, node_pointer> branch_data;
    typedef std::vector<branch_data> internal_heap_type;

    inline distance_query_incremental()
        : m_translator(NULL)
//        , m_pred()
        , current_neighbor((std::numeric_limits<size_type>::max)())
//        , m_strategy_type()
    {}

    inline distance_query_incremental(parameters_type const& params, translator_type const& translator, Predicates const& pred)
        : m_translator(::boost::addressof(translator))
        , m_pred(pred)
        , current_neighbor((std::numeric_limits<size_type>::max)())
        , m_strategy(index::detail::get_strategy(params))
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < max_count(), "k must be greather than 0");
    }

    const_reference dereference() const
    {
        return *(neighbors[current_neighbor].second);
    }

    void initialize(node_pointer root)
    {
        rtree::apply_visitor(*this, *root);
        increment();
    }

    void increment()
    {
        for (;;)
        {
            size_type new_neighbor = current_neighbor == (std::numeric_limits<size_type>::max)() ? 0 : current_neighbor + 1;

            if ( internal_heap.empty() )
            {
                if ( new_neighbor < neighbors.size() )
                    current_neighbor = new_neighbor;
                else
                {
                    current_neighbor = (std::numeric_limits<size_type>::max)();
                    // clear() is used to disable the condition above
                    neighbors.clear();
                }

                return;
            }
            else
            {
                branch_data const& closest_branch = internal_heap.front();
                node_distance_type const& closest_distance = closest_branch.first;

                // if there are no nodes which can have closer values, set new value
                if ( new_neighbor < neighbors.size() &&
                     // NOTE: In order to use <= current neighbor can't be sorted again
                     neighbors[new_neighbor].first <= closest_distance )
                {
                    current_neighbor = new_neighbor;
                    return;
                }

                // if node is further than the furthest neighbor, following nodes will also be further
                BOOST_GEOMETRY_INDEX_ASSERT(neighbors.size() <= max_count(), "unexpected neighbors count");
                if ( max_count() <= neighbors.size() &&
                     neighbors.back().first <= closest_distance )
                {
                    internal_heap.clear();
                    continue;
                }
                else
                {
                    node_pointer ptr = closest_branch.second;
                    std::pop_heap(internal_heap.begin(), internal_heap.end(), pair_first_greater());
                    internal_heap.pop_back();

                    rtree::apply_visitor(*this, *ptr);
                }
            }
        }
    }

    bool is_end() const
    {
        return (std::numeric_limits<size_type>::max)() == current_neighbor;
    }

    friend bool operator==(distance_query_incremental const& l, distance_query_incremental const& r)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(l.current_neighbor != r.current_neighbor ||
                                    (std::numeric_limits<size_type>::max)() == l.current_neighbor ||
                                    (std::numeric_limits<size_type>::max)() == r.current_neighbor ||
                                    l.neighbors[l.current_neighbor].second == r.neighbors[r.current_neighbor].second,
                                    "not corresponding iterators");
        return l.current_neighbor == r.current_neighbor;
    }

    // Put node's elements into the list of active branches if those elements meets predicates
    // and distance predicates(currently not used)
    // and aren't further than found neighbours (if there is enough neighbours)
    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        // fill active branch list array of nodes meeting predicates
        for ( typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it )
        {
            // if current node meets predicates
            // 0 - dummy value
            if ( index::detail::predicates_check
                    <
                        index::detail::bounds_tag, 0, predicates_len
                    >(m_pred, 0, it->first, m_strategy) )
            {
                // calculate node's distance(s) for distance predicate
                node_distance_type node_distance;
                // if distance isn't ok - move to the next node
                if ( !calculate_node_distance::apply(predicate(), it->first,
                                                     m_strategy, node_distance) )
                {
                    continue;
                }

                // if current node is further than found neighbors - don't analyze it
                if ( max_count() <= neighbors.size() &&
                     neighbors.back().first <= node_distance )
                {
                    continue;
                }

                // add current node's data into the queue
                internal_heap.push_back(std::make_pair(node_distance, it->second));
                std::push_heap(internal_heap.begin(), internal_heap.end(), pair_first_greater());
            }
        }
    }

    // Put values into the list of neighbours if those values meets predicates
    // and distance predicates(currently not used)
    // and aren't further than already found neighbours (if there is enough neighbours)
    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        // store distance to the furthest neighbour
        bool not_enough_neighbors = neighbors.size() < max_count();
        value_distance_type greatest_distance = !not_enough_neighbors ? neighbors.back().first : (std::numeric_limits<value_distance_type>::max)();
        
        // search leaf for closest value meeting predicates
        for ( typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            // if value meets predicates
            if ( index::detail::predicates_check
                    <
                        index::detail::value_tag, 0, predicates_len
                    >(m_pred, *it, (*m_translator)(*it), m_strategy) )
            {
                // calculate values distance for distance predicate
                value_distance_type value_distance;
                // if distance is ok
                if ( calculate_value_distance::apply(predicate(), (*m_translator)(*it),
                                                     m_strategy, value_distance) )
                {
                    // if there is not enough values or current value is closer than furthest neighbour
                    if ( not_enough_neighbors || value_distance < greatest_distance )
                    {
                        neighbors.push_back(std::make_pair(value_distance, boost::addressof(*it)));
                    }
                }
            }
        }

        // TODO: sort is probably suboptimal.
        //   An alternative would be std::set, but it'd probably add constant cost.
        //   Ideally replace this with double-ended priority queue, e.g. min-max heap.
        // NOTE: A condition in increment() relies on the fact that current neighbor doesn't
        //   participate in sorting anymore.

        // sort array
        size_type sort_first = current_neighbor == (std::numeric_limits<size_type>::max)() ? 0 : current_neighbor + 1;
        std::sort(neighbors.begin() + sort_first, neighbors.end(), pair_first_less());
        // remove furthest values
        if ( max_count() < neighbors.size() )
            neighbors.resize(max_count());
    }

private:
    inline std::size_t max_count() const
    {
        return nearest_predicate_access::get(m_pred).count;
    }

    nearest_predicate_type const& predicate() const
    {
        return nearest_predicate_access::get(m_pred);
    }

    const translator_type * m_translator;

    Predicates m_pred;
    
    internal_heap_type internal_heap;
    std::vector< std::pair<value_distance_type, const value_type *> > neighbors;
    size_type current_neighbor;

    strategy_type m_strategy;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DISTANCE_QUERY_HPP
