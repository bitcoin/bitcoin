//  (C) Copyright Gennadiy Rozental 2001.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : defines facility to hide input traversing details
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_CLA_ARGV_TRAVERSER_HPP
#define BOOST_TEST_UTILS_RUNTIME_CLA_ARGV_TRAVERSER_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/fwd.hpp>
#include <cstring>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {
namespace cla {

// ************************************************************************** //
// **************          runtime::cla::argv_traverser        ************** //
// ************************************************************************** //

class argv_traverser {
    typedef char const** argv_type;
public:
    /// Constructs traverser based on argc/argv pair
    /// argv is taken "by reference" and later can be
    /// updated in remainder method
    argv_traverser( int argc, argv_type argv )
    : m_argc( argc )
    , m_curr_token( 0 )
    , m_token_size( 0 )
    , m_argv( argv )
    {
        // save program name
        save_token();
    }

    /// Returns new argc
    int         remainder()
    {
        return static_cast<int>(m_argc);
    }

    /// Returns true, if we reached end on input
    bool        eoi() const
    {
        return m_curr_token == m_argc;
    }

    /// Returns current token in the input
    cstring     current_token()
    {
        if( eoi() )
            return cstring();

        return cstring( m_argv[m_curr_token], m_token_size );
    }

    /// Saves current token for remainder
    void        save_token()
    {
        ++m_curr_token;

        if( !eoi() )
            m_token_size = ::strlen( m_argv[m_curr_token] );
    }

    /// Commit current token and iterate to next one
    void        next_token()
    {
        if( !eoi() ) {
            for( std::size_t i = m_curr_token; i < m_argc-1; ++i )
                m_argv[i] = m_argv[i + 1];

            --m_argc;

            m_token_size = ::strlen( m_argv[m_curr_token] );
        }
    }

private:

    // Data members
    std::size_t m_argc;         // total number of arguments
    std::size_t m_curr_token;   // current token index in argv
    std::size_t m_token_size;   // current token size
    argv_type   m_argv;         // all arguments
};

} // namespace cla
} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_CLA_ARGV_TRAVERSER_HPP
