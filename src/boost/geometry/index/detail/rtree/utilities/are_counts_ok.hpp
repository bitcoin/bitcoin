// Boost.Geometry Index
//
// R-tree nodes elements numbers validating visitor implementation
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_ARE_COUNTS_OK_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_ARE_COUNTS_OK_HPP

#include <boost/geometry/index/detail/rtree/node/node.hpp>

namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree { namespace utilities {

namespace visitors {

template <typename MembersHolder>
class are_counts_ok
    : public MembersHolder::visitor_const
{
    typedef typename MembersHolder::parameters_type parameters_type;
    
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

public:
    inline are_counts_ok(parameters_type const& parameters, bool check_min = true)
        : result(true)
        , m_current_level(0)
        , m_parameters(parameters)
        , m_check_min(check_min)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        // root internal node shouldn't contain 0 elements
        if ( (elements.empty() && m_check_min)
          || !check_count(elements) )
        {
            result = false;
            return;
        }

        size_t current_level_backup = m_current_level;
        ++m_current_level;

        for ( typename elements_type::const_iterator it = elements.begin();
              it != elements.end() && result == true ;
              ++it)
        {
            rtree::apply_visitor(*this, *it->second);
        }

        m_current_level = current_level_backup;
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        // empty leaf in non-root node
        if ( (m_current_level > 0 && elements.empty() && m_check_min)
          || !check_count(elements) )
        {
            result = false;
        }
    }

    bool result;

private:
    template <typename Elements>
    bool check_count(Elements const& elements)
    {
        // root may contain count < min but should never contain count > max
        return elements.size() <= m_parameters.get_max_elements()
            && ( elements.size() >= m_parameters.get_min_elements()
              || m_current_level == 0 || !m_check_min );
    }

    size_t m_current_level;
    parameters_type const& m_parameters;
    bool m_check_min;
};

} // namespace visitors

template <typename Rtree> inline
bool are_counts_ok(Rtree const& tree, bool check_min = true)
{
    typedef utilities::view<Rtree> RTV;
    RTV rtv(tree);

    visitors::are_counts_ok<
        typename RTV::members_holder
    > v(tree.parameters(), check_min);
    
    rtv.apply_visitor(v);

    return v.result;
}

}}}}}} // namespace boost::geometry::index::detail::rtree::utilities

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_ARE_COUNTS_OK_HPP
