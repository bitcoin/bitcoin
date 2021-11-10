#ifndef BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED
#define BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED

//  http://www.boost.org/libs/assert
//
//  Copyright 2019 Peter Dimov
//  Distributed under the Boost Software License, Version 1.0.
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/current_function.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <iosfwd>

namespace boost
{

struct source_location
{
private:

    char const * file_;
    char const * function_;
    boost::uint_least32_t line_;
    boost::uint_least32_t column_;

public:

    BOOST_CONSTEXPR source_location() BOOST_NOEXCEPT: file_( "(unknown)" ), function_( "(unknown)" ), line_( 0 ), column_( 0 )
    {
    }

    BOOST_CONSTEXPR source_location( char const * file, boost::uint_least32_t ln, char const * function, boost::uint_least32_t col = 0 ) BOOST_NOEXCEPT: file_( file ), function_( function ), line_( ln ), column_( col )
    {
    }

    BOOST_CONSTEXPR char const * file_name() const BOOST_NOEXCEPT
    {
        return file_;
    }

    BOOST_CONSTEXPR char const * function_name() const BOOST_NOEXCEPT
    {
        return function_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t line() const BOOST_NOEXCEPT
    {
        return line_;
    }

    BOOST_CONSTEXPR boost::uint_least32_t column() const BOOST_NOEXCEPT
    {
        return column_;
    }
};

template<class E, class T> std::basic_ostream<E, T> & operator<<( std::basic_ostream<E, T> & os, source_location const & loc )
{
    os.width( 0 );

    if( loc.line() == 0 )
    {
        os << "(unknown source location)";
    }
    else
    {
        os << loc.file_name() << ':' << loc.line();

        if( loc.column() )
        {
            os << ':' << loc.column();
        }

        os << ": in function '" << loc.function_name() << '\'';
    }

    return os;
}

} // namespace boost

#if defined( BOOST_DISABLE_CURRENT_LOCATION )

#  define BOOST_CURRENT_LOCATION ::boost::source_location()

#elif defined(__clang_analyzer__)

// Cast to char const* to placate clang-tidy
// https://bugs.llvm.org/show_bug.cgi?id=28480
#  define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, static_cast<char const*>(BOOST_CURRENT_FUNCTION))

#else

#  define BOOST_CURRENT_LOCATION ::boost::source_location(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION)

#endif

#endif // #ifndef BOOST_ASSERT_SOURCE_LOCATION_HPP_INCLUDED
