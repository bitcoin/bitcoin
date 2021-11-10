#ifndef BOOST_ARCHIVE_ARCHIVE_EXCEPTION_HPP
#define BOOST_ARCHIVE_ARCHIVE_EXCEPTION_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// archive/archive_exception.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <exception>
#include <boost/assert.hpp>
#include <string>

#include <boost/config.hpp>
#include <boost/archive/detail/decl.hpp>

// note: the only reason this is in here is that windows header
// includes #define exception_code _exception_code (arrrgghhhh!).
// the most expedient way to address this is be sure that this
// header is always included whenever this header file is included.
#if defined(BOOST_WINDOWS)
#include <excpt.h>
#endif

#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {
namespace archive {

//////////////////////////////////////////////////////////////////////
// exceptions thrown by archives
//
class BOOST_SYMBOL_VISIBLE archive_exception :
    public virtual std::exception
{
private:
    char m_buffer[128];
protected:
    BOOST_ARCHIVE_DECL unsigned int
    append(unsigned int l, const char * a);
    BOOST_ARCHIVE_DECL
    archive_exception() BOOST_NOEXCEPT;
public:
    typedef enum {
        no_exception,       // initialized without code
        other_exception,    // any exception not listed below
        unregistered_class, // attempt to serialize a pointer of
                            // an unregistered class
        invalid_signature,  // first line of archive does not contain
                            // expected string
        unsupported_version,// archive created with library version
                            // subsequent to this one
        pointer_conflict,   // an attempt has been made to directly
                            // serialize an object which has
                            // already been serialized through a pointer.
                            // Were this permitted, the archive load would result
                            // in the creation of an extra copy of the object.
        incompatible_native_format, // attempt to read native binary format
                            // on incompatible platform
        array_size_too_short,// array being loaded doesn't fit in array allocated
        input_stream_error, // error on input stream
        invalid_class_name, // class name greater than the maximum permitted.
                            // most likely a corrupted archive or an attempt
                            // to insert virus via buffer overrun method.
        unregistered_cast,   // base - derived relationship not registered with
                            // void_cast_register
        unsupported_class_version, // type saved with a version # greater than the
                            // one used by the program.  This indicates that the program
                            // needs to be rebuilt.
        multiple_code_instantiation, // code for implementing serialization for some
                            // type has been instantiated in more than one module.
        output_stream_error // error on input stream
    } exception_code;
    exception_code code;

    BOOST_ARCHIVE_DECL archive_exception(
        exception_code c,
        const char * e1 = NULL,
        const char * e2 = NULL
    ) BOOST_NOEXCEPT;
    BOOST_ARCHIVE_DECL archive_exception(archive_exception const &) BOOST_NOEXCEPT;
    BOOST_ARCHIVE_DECL ~archive_exception() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE;
    BOOST_ARCHIVE_DECL const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE;
};

}// namespace archive
}// namespace boost

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif //BOOST_ARCHIVE_ARCHIVE_EXCEPTION_HPP
