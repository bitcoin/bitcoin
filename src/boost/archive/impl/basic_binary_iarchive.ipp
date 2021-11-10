/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_binary_iarchive.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.
#include <string>
#include <boost/assert.hpp>
#include <algorithm>
#include <cstring>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::memcpy; 
    using ::strlen;
    using ::size_t;
}
#endif

#include <boost/detail/workaround.hpp>
#include <boost/predef/other/endian.h>

#include <boost/archive/basic_binary_iarchive.hpp>

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implementation of binary_binary_archive
template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iarchive<Archive>::load_override(class_name_type & t){
    std::string cn;
    cn.reserve(BOOST_SERIALIZATION_MAX_KEY_SIZE);
    load_override(cn);
    if(cn.size() > (BOOST_SERIALIZATION_MAX_KEY_SIZE - 1))
        boost::serialization::throw_exception(
            archive_exception(archive_exception::invalid_class_name)
        );
    std::memcpy(t, cn.data(), cn.size());
    // borland tweak
    t.t[cn.size()] = '\0';
}

template<class Archive>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iarchive<Archive>::init() {
    // read signature in an archive version independent manner
    std::string file_signature;
    
    #if 0 // commented out since it interfers with derivation
    BOOST_TRY {
        std::size_t l;
        this->This()->load(l);
        if(l == std::strlen(BOOST_ARCHIVE_SIGNATURE())) {
            // borland de-allocator fixup
            #if BOOST_WORKAROUND(_RWSTD_VER, BOOST_TESTED_AT(20101))
            if(NULL != file_signature.data())
            #endif
                file_signature.resize(l);
            // note breaking a rule here - could be a problem on some platform
            if(0 < l)
                this->This()->load_binary(&(*file_signature.begin()), l);
        }
    }
    BOOST_CATCH(archive_exception const &) {  // catch stream_error archive exceptions
        // will cause invalid_signature archive exception to be thrown below
        file_signature = "";   
    }
    BOOST_CATCH_END
    #else
    // https://svn.boost.org/trac/boost/ticket/7301
    * this->This() >> file_signature;
    #endif

    if(file_signature != BOOST_ARCHIVE_SIGNATURE())
        boost::serialization::throw_exception(
            archive_exception(archive_exception::invalid_signature)
        );

    // make sure the version of the reading archive library can
    // support the format of the archive being read
    boost::serialization::library_version_type input_library_version;
    //* this->This() >> input_library_version;
    {
        int v = 0;
        v = this->This()->m_sb.sbumpc();
        #if BOOST_ENDIAN_LITTLE_BYTE
        if(v < 6){
            ;
        }
        else
        if(v < 7){
            // version 6 - next byte should be zero
            this->This()->m_sb.sbumpc();
        }
        else
        if(v < 8){
            int x1;
            // version 7 = might be followed by zero or some other byte
            x1 = this->This()->m_sb.sgetc();
            // it's =a zero, push it back
            if(0 == x1)
                this->This()->m_sb.sbumpc();
        }
        else{
            // version 8+ followed by a zero
            this->This()->m_sb.sbumpc();
        }
        #elif BOOST_ENDIAN_BIG_BYTE
        if(v == 0)
            v = this->This()->m_sb.sbumpc();
        #endif
        input_library_version = static_cast<boost::serialization::library_version_type>(v);
    }
    
    #if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))
    this->set_library_version(input_library_version);
    #else
    detail::basic_iarchive::set_library_version(input_library_version);
    #endif
    
    if(BOOST_ARCHIVE_VERSION() < input_library_version)
        boost::serialization::throw_exception(
            archive_exception(archive_exception::unsupported_version)
        );
}

} // namespace archive
} // namespace boost
