//            Copyright Daniel Trebbien 2010.
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or the copy at
//         http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_BUFFER_CONCEPTS_HPP
#define BOOST_GRAPH_BUFFER_CONCEPTS_HPP 1
#include <boost/concept_check.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/concept/detail/concept_def.hpp>
namespace boost
{

BOOST_concept(Buffer, (B))
{
    typedef typename B::value_type value_type;
    typedef typename B::size_type size_type;

    BOOST_CONCEPT_USAGE(Buffer)
    {
        typedef typename boost::add_reference< value_type >::type reference;

        BOOST_CONCEPT_ASSERT((Assignable< value_type >));

        buf.push(g_ct);
        buf.pop();
        reference t = buf.top();
        boost::ignore_unused_variable_warning(t);
    }

    void const_constraints(const B& cbuf)
    {
        typedef typename boost::add_const<
            typename boost::remove_reference< value_type >::type >::type&
            const_reference;

        const_reference ct = cbuf.top();
        s = cbuf.size();
        if (cbuf.empty())
            dummy = __LINE__;
    }

    int dummy;

    static const value_type g_ct;
    size_type s;
    B buf;
};

BOOST_concept(UpdatableQueue, (Q)) : Buffer< Q >
{
    BOOST_CONCEPT_USAGE(UpdatableQueue) { q.update(g_ct); }

    void const_constraints(const Q& cq)
    {
        if (cq.contains(g_ct))
            dummy = __LINE__;
    }

    int dummy;

    static const typename Buffer< Q >::value_type g_ct;
    Q q;
};

BOOST_concept(KeyedUpdatableQueue, (Q)) : UpdatableQueue< Q >
{
    typedef typename Q::key_type key_type;
    typedef typename Q::key_map key_map;

    BOOST_CONCEPT_USAGE(KeyedUpdatableQueue)
    {
        BOOST_CONCEPT_ASSERT((boost::ReadWritePropertyMapConcept< key_map,
            typename Buffer< Q >::value_type >));
    }

    void const_constraints(const Q& cq)
    {
        km = cq.keys();
        k = get(km, g_ct);
    }

    static const typename Buffer< Q >::value_type g_ct;
    key_type k;
    key_map km;
    Q q;
};

} // end `namespace boost`
#include <boost/concept/detail/concept_undef.hpp>

#endif // !BOOST_GRAPH_BUFFER_CONCEPTS_HPP
