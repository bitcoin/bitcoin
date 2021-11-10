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
//  Description : model of actual argument (both typed and abstract interface)
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_ARGUMENT_HPP
#define BOOST_TEST_UTILS_RUNTIME_ARGUMENT_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/fwd.hpp>
#include <boost/test/utils/runtime/errors.hpp>

// Boost.Test
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/rtti.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/detail/throw_exception.hpp>

// STL
#include <cassert>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {

// ************************************************************************** //
// **************              runtime::argument               ************** //
// ************************************************************************** //

class argument {
public:
    // Constructor
    argument( rtti::id_t value_type )
    : p_value_type( value_type )
    {}

    // Destructor
    virtual     ~argument()  {}

    // Public properties
    rtti::id_t const    p_value_type;
};

// ************************************************************************** //
// **************             runtime::typed_argument          ************** //
// ************************************************************************** //

template<typename T>
class typed_argument : public argument {
public:
    // Constructor
    explicit typed_argument( T const& v )
    : argument( rtti::type_id<T>() )
    , p_value( v )
    {}

    unit_test::readwrite_property<T>    p_value;
};

// ************************************************************************** //
// **************           runtime::arguments_store          ************** //
// ************************************************************************** //

class arguments_store {
public:
    typedef std::map<cstring, argument_ptr> storage_type;

    /// Returns number of arguments in the store; mostly used for testing
    std::size_t size() const        { return m_arguments.size(); }

    /// Clears the store for reuse
    void        clear()             { m_arguments.clear(); }

    /// Returns true if there is an argument corresponding to the specified parameter name
    bool        has( cstring parameter_name ) const
    {
        return m_arguments.find( parameter_name ) != m_arguments.end();
    }

    /// Provides types access to argument value by parameter name
    template<typename T>
    T const&    get( cstring parameter_name ) const {
        return const_cast<arguments_store*>(this)->get<T>( parameter_name );
    }

    template<typename T>
    T&          get( cstring parameter_name ) {
        storage_type::const_iterator found = m_arguments.find( parameter_name );
        BOOST_TEST_I_ASSRT( found != m_arguments.end(),
                            access_to_missing_argument() 
                                << "There is no argument provided for parameter "
                                << parameter_name );

        argument_ptr arg = found->second;

        BOOST_TEST_I_ASSRT( arg->p_value_type == rtti::type_id<T>(),
                            arg_type_mismatch()
                                << "Access with invalid type for argument corresponding to parameter "
                                << parameter_name );

        return static_cast<typed_argument<T>&>( *arg ).p_value.value;
    }

    /// Set's the argument value for specified parameter name
    template<typename T>
    void        set( cstring parameter_name, T const& value )
    {
        m_arguments[parameter_name] = argument_ptr( new typed_argument<T>( value ) );
    }

private:
    // Data members
    storage_type            m_arguments;
};

} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_ARGUMENT_HPP
