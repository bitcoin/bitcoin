// Boost.Geometry Index
//
// R-tree scoped deallocator
//
// Copyright (c) 2011-2018 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_SCOPED_DEALLOCATOR_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_SCOPED_DEALLOCATOR_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename Alloc>
class scoped_deallocator
{
    typedef boost::container::allocator_traits<Alloc> alloc_traits;

    scoped_deallocator(scoped_deallocator const&);
    scoped_deallocator & operator=(scoped_deallocator const&);
public:
    typedef typename alloc_traits::pointer pointer;

    inline scoped_deallocator(pointer p, Alloc & a)
        : m_ptr(p), m_alloc(a)
    {}
    inline ~scoped_deallocator()
    {
        if ( m_ptr )
        {
            alloc_traits::deallocate(m_alloc, m_ptr, 1);
        }
    }
    inline void release()
    {
        m_ptr = 0;
    }
private:
    pointer m_ptr;
    Alloc & m_alloc;
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_SCOPED_DEALLOCATOR_HPP
