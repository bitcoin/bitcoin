// Copyright 2009 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Andrew Lumsdaine

#ifndef BOOST_GRAPH_DETAIL_HISTOGRAM_SORT_HPP
#define BOOST_GRAPH_DETAIL_HISTOGRAM_SORT_HPP

#include <boost/assert.hpp>

namespace boost
{
namespace graph
{
    namespace detail
    {

        template < typename InputIterator >
        size_t reserve_count_for_single_pass_helper(
            InputIterator, InputIterator, std::input_iterator_tag)
        {
            // Do nothing: we have no idea how much storage to reserve.
            return 0;
        }

        template < typename InputIterator >
        size_t reserve_count_for_single_pass_helper(InputIterator first,
            InputIterator last, std::random_access_iterator_tag)
        {
            using std::distance;
            typename std::iterator_traits< InputIterator >::difference_type n
                = distance(first, last);
            return (size_t)n;
        }

        template < typename InputIterator >
        size_t reserve_count_for_single_pass(
            InputIterator first, InputIterator last)
        {
            typedef typename std::iterator_traits<
                InputIterator >::iterator_category category;
            return reserve_count_for_single_pass_helper(
                first, last, category());
        }

        template < typename KeyIterator, typename RowstartIterator,
            typename VerticesSize, typename KeyFilter, typename KeyTransform >
        void count_starts(KeyIterator begin, KeyIterator end,
            RowstartIterator starts, // Must support numverts + 1 elements
            VerticesSize numkeys, KeyFilter key_filter,
            KeyTransform key_transform)
        {

            typedef
                typename std::iterator_traits< RowstartIterator >::value_type
                    EdgeIndex;

            // Put the degree of each vertex v into m_rowstart[v + 1]
            for (KeyIterator i = begin; i != end; ++i)
            {
                if (key_filter(*i))
                {
                    BOOST_ASSERT(key_transform(*i) < numkeys);
                    ++starts[key_transform(*i) + 1];
                }
            }

            // Compute the partial sum of the degrees to get the actual values
            // of m_rowstart
            EdgeIndex start_of_this_row = 0;
            starts[0] = start_of_this_row;
            for (VerticesSize i = 1; i < numkeys + 1; ++i)
            {
                start_of_this_row += starts[i];
                starts[i] = start_of_this_row;
            }
        }

        template < typename KeyIterator, typename RowstartIterator,
            typename NumKeys, typename Value1InputIter,
            typename Value1OutputIter, typename KeyFilter,
            typename KeyTransform >
        void histogram_sort(KeyIterator key_begin, KeyIterator key_end,
            RowstartIterator rowstart, // Must support numkeys + 1 elements and
                                       // be precomputed
            NumKeys numkeys, Value1InputIter values1_begin,
            Value1OutputIter values1_out, KeyFilter key_filter,
            KeyTransform key_transform)
        {

            typedef
                typename std::iterator_traits< RowstartIterator >::value_type
                    EdgeIndex;

            // Histogram sort the edges by their source vertices, putting the
            // targets into m_column.  The index current_insert_positions[v]
            // contains the next location to insert out edges for vertex v.
            std::vector< EdgeIndex > current_insert_positions(
                rowstart, rowstart + numkeys);
            Value1InputIter v1i = values1_begin;
            for (KeyIterator i = key_begin; i != key_end; ++i, ++v1i)
            {
                if (key_filter(*i))
                {
                    NumKeys source = key_transform(*i);
                    BOOST_ASSERT(source < numkeys);
                    EdgeIndex insert_pos = current_insert_positions[source];
                    ++current_insert_positions[source];
                    values1_out[insert_pos] = *v1i;
                }
            }
        }

        template < typename KeyIterator, typename RowstartIterator,
            typename NumKeys, typename Value1InputIter,
            typename Value1OutputIter, typename Value2InputIter,
            typename Value2OutputIter, typename KeyFilter,
            typename KeyTransform >
        void histogram_sort(KeyIterator key_begin, KeyIterator key_end,
            RowstartIterator rowstart, // Must support numkeys + 1 elements and
                                       // be precomputed
            NumKeys numkeys, Value1InputIter values1_begin,
            Value1OutputIter values1_out, Value2InputIter values2_begin,
            Value2OutputIter values2_out, KeyFilter key_filter,
            KeyTransform key_transform)
        {

            typedef
                typename std::iterator_traits< RowstartIterator >::value_type
                    EdgeIndex;

            // Histogram sort the edges by their source vertices, putting the
            // targets into m_column.  The index current_insert_positions[v]
            // contains the next location to insert out edges for vertex v.
            std::vector< EdgeIndex > current_insert_positions(
                rowstart, rowstart + numkeys);
            Value1InputIter v1i = values1_begin;
            Value2InputIter v2i = values2_begin;
            for (KeyIterator i = key_begin; i != key_end; ++i, ++v1i, ++v2i)
            {
                if (key_filter(*i))
                {
                    NumKeys source = key_transform(*i);
                    BOOST_ASSERT(source < numkeys);
                    EdgeIndex insert_pos = current_insert_positions[source];
                    ++current_insert_positions[source];
                    values1_out[insert_pos] = *v1i;
                    values2_out[insert_pos] = *v2i;
                }
            }
        }

        template < typename KeyIterator, typename RowstartIterator,
            typename NumKeys, typename Value1Iter, typename KeyTransform >
        void histogram_sort_inplace(KeyIterator key_begin,
            RowstartIterator rowstart, // Must support numkeys + 1 elements and
                                       // be precomputed
            NumKeys numkeys, Value1Iter values1, KeyTransform key_transform)
        {

            typedef
                typename std::iterator_traits< RowstartIterator >::value_type
                    EdgeIndex;

            // 1. Copy m_rowstart (except last element) to get insert positions
            std::vector< EdgeIndex > insert_positions(
                rowstart, rowstart + numkeys);
            // 2. Swap the sources and targets into place
            for (size_t i = 0; i < rowstart[numkeys]; ++i)
            {
                BOOST_ASSERT(key_transform(key_begin[i]) < numkeys);
                // While edge i is not in the right bucket:
                while (!(i >= rowstart[key_transform(key_begin[i])]
                    && i < insert_positions[key_transform(key_begin[i])]))
                {
                    // Add a slot in the right bucket
                    size_t target_pos
                        = insert_positions[key_transform(key_begin[i])]++;
                    BOOST_ASSERT(
                        target_pos < rowstart[key_transform(key_begin[i]) + 1]);
                    if (target_pos == i)
                        continue;
                    // Swap this edge into place
                    using std::swap;
                    swap(key_begin[i], key_begin[target_pos]);
                    swap(values1[i], values1[target_pos]);
                }
            }
        }

        template < typename KeyIterator, typename RowstartIterator,
            typename NumKeys, typename Value1Iter, typename Value2Iter,
            typename KeyTransform >
        void histogram_sort_inplace(KeyIterator key_begin,
            RowstartIterator rowstart, // Must support numkeys + 1 elements and
                                       // be precomputed
            NumKeys numkeys, Value1Iter values1, Value2Iter values2,
            KeyTransform key_transform)
        {

            typedef
                typename std::iterator_traits< RowstartIterator >::value_type
                    EdgeIndex;

            // 1. Copy m_rowstart (except last element) to get insert positions
            std::vector< EdgeIndex > insert_positions(
                rowstart, rowstart + numkeys);
            // 2. Swap the sources and targets into place
            for (size_t i = 0; i < rowstart[numkeys]; ++i)
            {
                BOOST_ASSERT(key_transform(key_begin[i]) < numkeys);
                // While edge i is not in the right bucket:
                while (!(i >= rowstart[key_transform(key_begin[i])]
                    && i < insert_positions[key_transform(key_begin[i])]))
                {
                    // Add a slot in the right bucket
                    size_t target_pos
                        = insert_positions[key_transform(key_begin[i])]++;
                    BOOST_ASSERT(
                        target_pos < rowstart[key_transform(key_begin[i]) + 1]);
                    if (target_pos == i)
                        continue;
                    // Swap this edge into place
                    using std::swap;
                    swap(key_begin[i], key_begin[target_pos]);
                    swap(values1[i], values1[target_pos]);
                    swap(values2[i], values2[target_pos]);
                }
            }
        }

        template < typename InputIterator, typename VerticesSize >
        void split_into_separate_coords(InputIterator begin, InputIterator end,
            std::vector< VerticesSize >& firsts,
            std::vector< VerticesSize >& seconds)
        {
            firsts.clear();
            seconds.clear();
            size_t reserve_size
                = detail::reserve_count_for_single_pass(begin, end);
            firsts.reserve(reserve_size);
            seconds.reserve(reserve_size);
            for (; begin != end; ++begin)
            {
                std::pair< VerticesSize, VerticesSize > edge = *begin;
                firsts.push_back(edge.first);
                seconds.push_back(edge.second);
            }
        }

        template < typename InputIterator, typename VerticesSize,
            typename SourceFilter >
        void split_into_separate_coords_filtered(InputIterator begin,
            InputIterator end, std::vector< VerticesSize >& firsts,
            std::vector< VerticesSize >& seconds, const SourceFilter& filter)
        {
            firsts.clear();
            seconds.clear();
            for (; begin != end; ++begin)
            {
                std::pair< VerticesSize, VerticesSize > edge = *begin;
                if (filter(edge.first))
                {
                    firsts.push_back(edge.first);
                    seconds.push_back(edge.second);
                }
            }
        }

        template < typename InputIterator, typename PropInputIterator,
            typename VerticesSize, typename PropType, typename SourceFilter >
        void split_into_separate_coords_filtered(InputIterator begin,
            InputIterator end, PropInputIterator props,
            std::vector< VerticesSize >& firsts,
            std::vector< VerticesSize >& seconds,
            std::vector< PropType >& props_out, const SourceFilter& filter)
        {
            firsts.clear();
            seconds.clear();
            props_out.clear();
            for (; begin != end; ++begin)
            {
                std::pair< VerticesSize, VerticesSize > edge = *begin;
                if (filter(edge.first))
                {
                    firsts.push_back(edge.first);
                    seconds.push_back(edge.second);
                    props_out.push_back(*props);
                }
                ++props;
            }
        }

        // The versions of operator()() here can't return by reference because
        // the actual type passed in may not match Pair, in which case the
        // reference parameter is bound to a temporary that could end up
        // dangling after the operator returns.

        template < typename Pair > struct project1st
        {
            typedef typename Pair::first_type result_type;
            result_type operator()(const Pair& p) const { return p.first; }
        };

        template < typename Pair > struct project2nd
        {
            typedef typename Pair::second_type result_type;
            result_type operator()(const Pair& p) const { return p.second; }
        };

    }
}
}

#endif // BOOST_GRAPH_DETAIL_HISTOGRAM_SORT_HPP
