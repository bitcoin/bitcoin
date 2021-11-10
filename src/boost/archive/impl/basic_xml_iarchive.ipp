/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_xml_iarchive.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/assert.hpp>
#include <cstddef> // NULL
#include <algorithm>

#include <boost/serialization/throw_exception.hpp>
#include <boost/archive/xml_archive_exception.hpp>
#include <boost/archive/basic_xml_iarchive.hpp>
#include <boost/serialization/tracking.hpp>

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implementation of xml_text_archive

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_start(const char *name){
    // if there's no name
    if(NULL == name)
        return;
    bool result = this->This()->gimpl->parse_start_tag(this->This()->get_is());
    if(true != result){
        boost::serialization::throw_exception(
            archive_exception(archive_exception::input_stream_error)
        );
    }
    // don't check start tag at highest level
    ++depth;
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_end(const char *name){
    // if there's no name
    if(NULL == name)
        return;
    bool result = this->This()->gimpl->parse_end_tag(this->This()->get_is());
    if(true != result){
        boost::serialization::throw_exception(
            archive_exception(archive_exception::input_stream_error)
        );
    }
    
    // don't check start tag at highest level
    if(0 == --depth)
        return;
        
    if(0 == (this->get_flags() & no_xml_tag_checking)){
        // double check that the tag matches what is expected - useful for debug
        if(0 != name[this->This()->gimpl->rv.object_name.size()]
        || ! std::equal(
                this->This()->gimpl->rv.object_name.begin(),
                this->This()->gimpl->rv.object_name.end(),
                name
            )
        ){
            boost::serialization::throw_exception(
                xml_archive_exception(
                    xml_archive_exception::xml_archive_tag_mismatch,
                    name
                )
            );
        }
    }
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_override(object_id_type & t){
    t = object_id_type(this->This()->gimpl->rv.object_id);
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_override(version_type & t){
    t = version_type(this->This()->gimpl->rv.version);
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_override(class_id_type & t){
    t = class_id_type(this->This()->gimpl->rv.class_id);
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_xml_iarchive<Archive>::load_override(tracking_type & t){
    t = this->This()->gimpl->rv.tracking_level;
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_xml_iarchive<Archive>::basic_xml_iarchive(unsigned int flags) :
    detail::common_iarchive<Archive>(flags),
    depth(0)
{}
template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_xml_iarchive<Archive>::~basic_xml_iarchive(){
}

} // namespace archive
} // namespace boost
