/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// archive_serializer_map.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

//////////////////////////////////////////////////////////////////////
// implementation of basic_text_iprimitive overrides for the combination
// of template parameters used to implement a text_iprimitive

#include <boost/config.hpp>
#include <boost/archive/detail/archive_serializer_map.hpp>
#include <boost/archive/detail/basic_serializer_map.hpp>
#include <boost/serialization/singleton.hpp>

namespace boost {
namespace archive {
namespace detail {

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace extra_detail { // anon
    template<class Archive>
    class map : public basic_serializer_map 
    {};
}

#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL bool
archive_serializer_map<Archive>::insert(const basic_serializer * bs){
    return boost::serialization::singleton<
        extra_detail::map<Archive>
    >::get_mutable_instance().insert(bs);
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
archive_serializer_map<Archive>::erase(const basic_serializer * bs){
    // note: previously this conditional was a runtime assertion with
    // BOOST_ASSERT.  We've changed it because we've discovered that at
    // least one platform is not guaranteed to destroy singletons in
    // reverse order of distruction.
    if(boost::serialization::singleton<
        extra_detail::map<Archive>
    >::is_destroyed())
        return;
    boost::serialization::singleton<
        extra_detail::map<Archive>
    >::get_mutable_instance().erase(bs);
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL const basic_serializer *
archive_serializer_map<Archive>::find(
    const boost::serialization::extended_type_info & eti
) {
    return boost::serialization::singleton<
        extra_detail::map<Archive>
    >::get_const_instance().find(eti);
}

} // namespace detail
} // namespace archive
} // namespace boost
