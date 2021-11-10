// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Acknowledgements:
// aschoedl supplied a fix to supply the level of interoperability I had
// originally intended, but failed to implement.
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_DEMOTE_ITERATOR_TRAVERSAL_TAG_HPP_INCLUDED
#define BOOST_RANGE_DETAIL_DEMOTE_ITERATOR_TRAVERSAL_TAG_HPP_INCLUDED

#include <boost/iterator/iterator_categories.hpp>

namespace boost
{
    namespace range_detail
    {

template<class IteratorTraversalTag1, class IteratorTraversalTag2>
struct inner_demote_iterator_traversal_tag
{
};

#define BOOST_DEMOTE_TRAVERSAL_TAG( Tag1, Tag2, ResultTag ) \
template<> struct inner_demote_iterator_traversal_tag< Tag1 , Tag2 > \
{ \
    typedef ResultTag type; \
};

BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, no_traversal_tag,            no_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, incrementable_traversal_tag, no_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, single_pass_traversal_tag,   no_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, forward_traversal_tag,       no_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, bidirectional_traversal_tag, no_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( no_traversal_tag, random_access_traversal_tag, no_traversal_tag )

BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, no_traversal_tag,            no_traversal_tag            )
BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, incrementable_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, single_pass_traversal_tag,   incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, forward_traversal_tag,       incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, bidirectional_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( incrementable_traversal_tag, random_access_traversal_tag, incrementable_traversal_tag )

BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, no_traversal_tag,            no_traversal_tag            )
BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, incrementable_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, single_pass_traversal_tag,   single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, forward_traversal_tag,       single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, bidirectional_traversal_tag, single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( single_pass_traversal_tag, random_access_traversal_tag, single_pass_traversal_tag   )

BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, no_traversal_tag,            no_traversal_tag            )
BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, incrementable_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, single_pass_traversal_tag,   single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, forward_traversal_tag,       forward_traversal_tag       )
BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, bidirectional_traversal_tag, forward_traversal_tag       )
BOOST_DEMOTE_TRAVERSAL_TAG( forward_traversal_tag, random_access_traversal_tag, forward_traversal_tag       )

BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, no_traversal_tag,            no_traversal_tag            )
BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, incrementable_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, single_pass_traversal_tag,   single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, forward_traversal_tag,       forward_traversal_tag       )
BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, bidirectional_traversal_tag, bidirectional_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( bidirectional_traversal_tag, random_access_traversal_tag, bidirectional_traversal_tag )

BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, no_traversal_tag,            no_traversal_tag            )
BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, incrementable_traversal_tag, incrementable_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, single_pass_traversal_tag,   single_pass_traversal_tag   )
BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, forward_traversal_tag,       forward_traversal_tag       )
BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, bidirectional_traversal_tag, bidirectional_traversal_tag )
BOOST_DEMOTE_TRAVERSAL_TAG( random_access_traversal_tag, random_access_traversal_tag, random_access_traversal_tag )

#undef BOOST_DEMOTE_TRAVERSAL_TAG

template<class IteratorTraversalTag1, class IteratorTraversalTag2>
struct demote_iterator_traversal_tag
    : inner_demote_iterator_traversal_tag<
        typename boost::iterators::pure_traversal_tag< IteratorTraversalTag1 >::type,
        typename boost::iterators::pure_traversal_tag< IteratorTraversalTag2 >::type
      >
{
};

    } // namespace range_detail
} // namespace boost

#endif // include guard
