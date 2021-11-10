//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : defines runtime parameters setup error
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_INIT_ERROR_HPP
#define BOOST_TEST_UTILS_RUNTIME_INIT_ERROR_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/fwd.hpp>

// Boost.Test
#include <boost/test/utils/string_cast.hpp>

// Boost.Test
#include <boost/config.hpp>

// STL
#include <exception>
#include <vector>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {

// ************************************************************************** //
// **************             runtime::param_error             ************** //
// ************************************************************************** //

class BOOST_SYMBOL_VISIBLE param_error : public std::exception {
public:
    ~param_error() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
    {
        return msg.c_str();
    }

    cstring     param_name;
    std::string msg;

protected:
    explicit    param_error( cstring param_name_ ) : param_name( param_name_) {}
};

//____________________________________________________________________________//

class BOOST_SYMBOL_VISIBLE init_error : public param_error {
protected:
    explicit    init_error( cstring param_name ) : param_error( param_name ) {}
    ~init_error() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}
};

class BOOST_SYMBOL_VISIBLE input_error : public param_error {
protected:
    explicit    input_error( cstring param_name ) : param_error( param_name ) {}
    ~input_error() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}
};

//____________________________________________________________________________//

template<typename Derived, typename Base>
class BOOST_SYMBOL_VISIBLE specific_param_error : public Base {
protected:
    explicit    specific_param_error( cstring param_name ) : Base( param_name ) {}
    ~specific_param_error() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

public:

//____________________________________________________________________________//

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_REF_QUALIFIERS)

    Derived operator<<(char const* val) &&
    {
        this->msg.append( val );

        return static_cast<Derived&&>(*this);
    }

    //____________________________________________________________________________//

    template<typename T>
    Derived operator<<(T const& val) &&
    {
        this->msg.append( unit_test::utils::string_cast( val ) );

        return static_cast<Derived&&>(*this);
    }

    //____________________________________________________________________________//

#else

    Derived const& operator<<(char const* val) const
    {
        const_cast<specific_param_error<Derived, Base>&>(*this).msg.append( val );

        return static_cast<Derived const&>(*this);
    }

    //____________________________________________________________________________//

    template<typename T>
    Derived const& operator<<(T const& val) const
    {
        const_cast<specific_param_error<Derived, Base>&>(*this).msg.append( unit_test::utils::string_cast( val ) );

        return static_cast<Derived const&>(*this);
    }

    //____________________________________________________________________________//

#endif

};



// ************************************************************************** //
// **************           specific exception types           ************** //
// ************************************************************************** //

#define SPECIFIC_EX_TYPE( type, base )                  \
class BOOST_SYMBOL_VISIBLE type : public specific_param_error<type,base> {   \
public:                                                 \
    explicit type( cstring param_name = cstring() )     \
    : specific_param_error<type,base>( param_name )     \
    {}                                                  \
}                                                       \
/**/

SPECIFIC_EX_TYPE( invalid_cla_id, init_error );
SPECIFIC_EX_TYPE( duplicate_param, init_error );
SPECIFIC_EX_TYPE( conflicting_param, init_error );
SPECIFIC_EX_TYPE( unknown_param, init_error );
SPECIFIC_EX_TYPE( access_to_missing_argument, init_error );
SPECIFIC_EX_TYPE( arg_type_mismatch, init_error );
SPECIFIC_EX_TYPE( invalid_param_spec, init_error );

SPECIFIC_EX_TYPE( format_error, input_error );
SPECIFIC_EX_TYPE( duplicate_arg, input_error );
SPECIFIC_EX_TYPE( missing_req_arg, input_error );

#undef SPECIFIC_EX_TYPE

class BOOST_SYMBOL_VISIBLE ambiguous_param : public specific_param_error<ambiguous_param, input_error> {
public:
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    explicit    ambiguous_param( std::vector<cstring>&& amb_candidates )
    : specific_param_error<ambiguous_param,input_error>( "" )
    , m_amb_candidates( std::move( amb_candidates ) ) {}
#else
    explicit    ambiguous_param( std::vector<cstring> const& amb_candidates )
    : specific_param_error<ambiguous_param,input_error>( "" )
    , m_amb_candidates( amb_candidates ) {}
#endif
    ~ambiguous_param() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

    std::vector<cstring> m_amb_candidates;
};

class BOOST_SYMBOL_VISIBLE unrecognized_param : public specific_param_error<unrecognized_param, input_error> {
public:
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    explicit    unrecognized_param( std::vector<cstring>&& type_candidates )
    : specific_param_error<unrecognized_param,input_error>( "" )
    , m_typo_candidates( std::move( type_candidates ) ) {}
#else
    explicit    unrecognized_param( std::vector<cstring> const& type_candidates )
    : specific_param_error<unrecognized_param,input_error>( "" )
    , m_typo_candidates( type_candidates ) {}
#endif
    ~unrecognized_param() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

    std::vector<cstring> m_typo_candidates;
};

} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_INIT_ERROR_HPP
