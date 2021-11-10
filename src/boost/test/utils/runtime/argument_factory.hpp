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
//  Description : argument factories for different kinds of parameters
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_ARGUMENT_FACTORY_HPP
#define BOOST_TEST_UTILS_RUNTIME_ARGUMENT_FACTORY_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/errors.hpp>
#include <boost/test/utils/runtime/argument.hpp>
#include <boost/test/utils/runtime/modifier.hpp>

// Boost.Test
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/utils/string_cast.hpp>

// Boost
#include <boost/function/function2.hpp>

// STL
#include <vector>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {

// ************************************************************************** //
// **************          runtime::value_interpreter          ************** //
// ************************************************************************** //

template<typename ValueType, bool is_enum>
struct value_interpreter;

//____________________________________________________________________________//

template<typename ValueType>
struct value_interpreter<ValueType, false> {
    template<typename Modifiers>
    explicit    value_interpreter( Modifiers const& ) {}

    ValueType interpret( cstring param_name, cstring source ) const
    {
        ValueType res;
        if( !unit_test::utils::string_as<ValueType>( source, res ) )
            BOOST_TEST_I_THROW( format_error( param_name ) << source <<
                                " can't be interpreted as value of parameter " << param_name << "." );
        return res;
    }
};

//____________________________________________________________________________//

template<>
struct value_interpreter<std::string, false> {
    template<typename Modifiers>
    explicit    value_interpreter( Modifiers const& ) {}

    std::string interpret( cstring, cstring source ) const
    {
        return std::string( source.begin(), source.size() );
    }
};

//____________________________________________________________________________//

template<>
struct value_interpreter<cstring, false> {
    template<typename Modifiers>
    explicit    value_interpreter( Modifiers const& ) {}

    cstring interpret( cstring, cstring source ) const
    {
        return source;
    }
};

//____________________________________________________________________________//

template<>
struct value_interpreter<bool, false> {
    template<typename Modifiers>
    explicit    value_interpreter( Modifiers const& ) {}

    bool    interpret( cstring param_name, cstring source ) const
    {
        static cstring const s_YES( "YES" );
        static cstring const s_Y( "Y" );
        static cstring const s_NO( "NO" );
        static cstring const s_N( "N" );
        static cstring const s_TRUE( "TRUE" );
        static cstring const s_FALSE( "FALSE" );
        static cstring const s_one( "1" );
        static cstring const s_zero( "0" );

        source.trim();

        if( source.is_empty() ||
            case_ins_eq( source, s_YES ) ||
            case_ins_eq( source, s_Y ) ||
            case_ins_eq( source, s_one ) ||
            case_ins_eq( source, s_TRUE ) )
            return true;

        if( case_ins_eq( source, s_NO ) ||
            case_ins_eq( source, s_N ) ||
            case_ins_eq( source, s_zero ) ||
            case_ins_eq( source, s_FALSE ) )
            return false;

        BOOST_TEST_I_THROW( format_error( param_name ) << source << " can't be interpreted as bool value." );
    }
};

//____________________________________________________________________________//

template<typename EnumType>
struct value_interpreter<EnumType, true> {
    template<typename Modifiers>
    explicit        value_interpreter( Modifiers const& m )
#if defined(BOOST_TEST_CLA_NEW_API)
    : m_name_to_value( m[enum_values<EnumType>::value] )
    {
    }
#else
    {
        std::vector<std::pair<cstring,EnumType> > const& values = m[enum_values<EnumType>::value];

        m_name_to_value.insert( values.begin(), values.end() );
    }
#endif

    EnumType        interpret( cstring param_name, cstring source ) const
    {
        typename std::map<cstring,EnumType>::const_iterator found = m_name_to_value.find( source );

        BOOST_TEST_I_ASSRT( found != m_name_to_value.end(),
                            format_error( param_name ) << source <<
                            " is not a valid enumeration value name for parameter " << param_name << "." );

        return found->second;
    }

private:
    // Data members
    std::map<cstring,EnumType>  m_name_to_value;
};

//____________________________________________________________________________//

// ************************************************************************** //
// **************           runtime::argument_factory          ************** //
// ************************************************************************** //

template<typename ValueType, bool is_enum, bool repeatable>
class argument_factory;

//____________________________________________________________________________//

template<typename ValueType, bool is_enum>
class argument_factory<ValueType, is_enum, false> {
public:
    template<typename Modifiers>
    explicit    argument_factory( Modifiers const& m )
    : m_interpreter( m )
    , m_optional_value( nfp::opt_get( m, optional_value, ValueType() ) )
    , m_default_value( nfp::opt_get( m, default_value, ValueType() ) )
    {
    }

    void        produce_argument( cstring source, cstring param_name, arguments_store& store ) const
    {
        store.set( param_name, source.empty() ? m_optional_value : m_interpreter.interpret( param_name, source ) );
    }

    void        produce_default( cstring param_name, arguments_store& store ) const
    {
        store.set( param_name, m_default_value );
    }

private:
    // Data members
    typedef value_interpreter<ValueType, is_enum> interp_t;
    interp_t    m_interpreter;
    ValueType   m_optional_value;
    ValueType   m_default_value;
};

//____________________________________________________________________________//

template<typename ValueType, bool is_enum>
class argument_factory<ValueType, is_enum, true> {
public:
    template<typename Modifiers>
    explicit    argument_factory( Modifiers const& m )
    : m_interpreter( m )
    {
    }

    void        produce_argument( cstring source, cstring param_name, arguments_store& store ) const
    {
        ValueType value = m_interpreter.interpret( param_name, source );

        if( store.has( param_name ) ) {
            std::vector<ValueType>& values = store.get<std::vector<ValueType> >( param_name );
            values.push_back( value );
        }
        else {
            std::vector<ValueType> values( 1, value );

            store.set( param_name, values );
        }

    }
    void        produce_default( cstring param_name, arguments_store& store ) const
    {
        store.set( param_name, std::vector<ValueType>() );
    }

private:
    // Data members
    value_interpreter<ValueType, is_enum> m_interpreter;
};

//____________________________________________________________________________//

} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_ARGUMENT_FACTORY_HPP
