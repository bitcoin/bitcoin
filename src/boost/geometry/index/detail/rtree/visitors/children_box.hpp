// Boost.Geometry Index
//
// R-tree node children box calculating visitor implementation
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

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_CHILDREN_BOX_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_CHILDREN_BOX_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

template <typename MembersHolder>
class children_box
    : public MembersHolder::visitor_const
{
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::box_type box_type;

    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

public:
    inline children_box(box_type & result,
                        parameters_type const& parameters,
                        translator_type const& tr)
        : m_result(result), m_parameters(parameters), m_tr(tr)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        m_result = rtree::elements_box<box_type>(elements.begin(), elements.end(), m_tr,
                                                 index::detail::get_strategy(m_parameters));
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        m_result = rtree::values_box<box_type>(elements.begin(), elements.end(), m_tr,
                                               index::detail::get_strategy(m_parameters));
    }

private:
    box_type & m_result;
    parameters_type const& m_parameters;
    translator_type const& m_tr;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_CHILDREN_BOX_HPP
