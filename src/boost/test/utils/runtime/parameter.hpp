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
//  Description : formal parameter definition
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP
#define BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/fwd.hpp>
#include <boost/test/utils/runtime/modifier.hpp>
#include <boost/test/utils/runtime/argument.hpp>
#include <boost/test/utils/runtime/argument_factory.hpp>

// Boost.Test
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/foreach.hpp>
#include <boost/test/utils/setcolor.hpp>

// Boost
#include <boost/function.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

// STL
#include <algorithm>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {

inline
std::ostream& commandline_pretty_print(
    std::ostream& ostr, 
    std::string const& prefix, 
    std::string const& to_print) {
    
    const int split_at = 80;

    std::string::size_type current = 0;

    while(current < to_print.size()) {

        // discards spaces at the beginning
        std::string::size_type startpos = to_print.find_first_not_of(" \t\n", current);
        current += startpos - current;

        bool has_more_lines = (current + split_at) < to_print.size();

        if(has_more_lines) {
          std::string::size_type endpos = to_print.find_last_of(" \t\n", current + split_at);
          std::string sub(to_print.substr(current, endpos - current));
          ostr << prefix << sub;
          ostr << "\n";
          current += endpos - current;
        }
        else 
        {
          ostr << prefix << to_print.substr(current, split_at);
          current += split_at;
        }
    }
    return ostr;
}

// ************************************************************************** //
// **************           runtime::parameter_cla_id          ************** //
// ************************************************************************** //
// set of attributes identifying the parameter in the command line

struct parameter_cla_id {
    parameter_cla_id( cstring prefix, cstring tag, cstring value_separator, bool negatable )
    : m_prefix( prefix.begin(), prefix.size() )
    , m_tag( tag.begin(), tag.size() )
    , m_value_separator( value_separator.begin(), value_separator.size() )
    , m_negatable( negatable )
    {

        BOOST_TEST_I_ASSRT( algorithm::all_of( m_prefix.begin(), m_prefix.end(), valid_prefix_char ),
                            invalid_cla_id() << "Parameter " << m_tag
                                             << " has invalid characters in prefix." );

        BOOST_TEST_I_ASSRT( algorithm::all_of( m_tag.begin(), m_tag.end(), valid_name_char ),
                            invalid_cla_id() << "Parameter " << m_tag
                                             << " has invalid characters in name." );

        BOOST_TEST_I_ASSRT( algorithm::all_of( m_value_separator.begin(), m_value_separator.end(), valid_separator_char ),
                            invalid_cla_id() << "Parameter " << m_tag
                                             << " has invalid characters in value separator." );
    }

    static bool             valid_prefix_char( char c )
    {
        return c == '-' || c == '/' ;
    }
    static bool             valid_separator_char( char c )
    {
        return c == '=' || c == ':' || c == ' ' || c == '\0';
    }
    static bool             valid_name_char( char c )
    {
        return std::isalnum( c ) || c == '+' || c == '_' || c == '?';
    }

    std::string             m_prefix;
    std::string             m_tag;
    std::string             m_value_separator;
    bool                    m_negatable;
};

typedef std::vector<parameter_cla_id> param_cla_ids;

// ************************************************************************** //
// **************             runtime::basic_param             ************** //
// ************************************************************************** //

cstring const help_prefix("////");

class basic_param {
    typedef function<void (cstring)>            callback_type;
    typedef unit_test::readwrite_property<bool> bool_property;

protected:
    /// Constructor with modifiers
    template<typename Modifiers>
    basic_param( cstring name, bool is_optional, bool is_repeatable, Modifiers const& m )
    : p_name( name.begin(), name.end() )
    , p_description( nfp::opt_get( m, description, std::string() ) )
    , p_help( nfp::opt_get( m, runtime::help, std::string() ) )
    , p_env_var( nfp::opt_get( m, env_var, std::string() ) )
    , p_value_hint( nfp::opt_get( m, value_hint, std::string() ) )
    , p_optional( is_optional )
    , p_repeatable( is_repeatable )
    , p_has_optional_value( m.has( optional_value ) )
    , p_has_default_value( m.has( default_value ) || is_repeatable )
    , p_callback( nfp::opt_get( m, callback, callback_type() ) )
    {
        add_cla_id( help_prefix, name, ":" );
    }

public:
    virtual                 ~basic_param() {}

    // Pubic properties
    std::string const       p_name;
    std::string const       p_description;
    std::string const       p_help;
    std::string const       p_env_var;
    std::string const       p_value_hint;
    bool const              p_optional;
    bool const              p_repeatable;
    bool_property           p_has_optional_value;
    bool_property           p_has_default_value;
    callback_type const     p_callback;

    /// interface for cloning typed parameters
    virtual basic_param_ptr clone() const = 0;

    /// Access methods
    param_cla_ids const&    cla_ids() const { return m_cla_ids; }
    void                    add_cla_id( cstring prefix, cstring tag, cstring value_separator )
    {
        add_cla_id_impl( prefix, tag, value_separator, false, true );
    }

    /// interface for producing argument values for this parameter
    virtual void            produce_argument( cstring token, bool negative_form, arguments_store& store ) const = 0;
    virtual void            produce_default( arguments_store& store ) const = 0;

    /// interfaces for help message reporting
    virtual void            usage( std::ostream& ostr, cstring negation_prefix_, bool use_color = true )
    {
        namespace utils = unit_test::utils;
        namespace ut_detail = unit_test::ut_detail;

        // 
        ostr  << "  ";
        {

          BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::GREEN );
          ostr << p_name;
        }

        ostr << '\n';

        if( !p_description.empty() ) {
          commandline_pretty_print(ostr, "    ", p_description) << '\n';
        }

        BOOST_TEST_FOREACH( parameter_cla_id const&, id, cla_ids() ) {
            if( id.m_prefix == help_prefix )
                continue;

            ostr << "    " << id.m_prefix;

            if( id.m_negatable )
                cla_name_help( ostr, id.m_tag, negation_prefix_, use_color );
            else
                cla_name_help( ostr, id.m_tag, "", use_color );

            BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::YELLOW );
            bool optional_value_ = false;

            if( p_has_optional_value ) {
                optional_value_ = true;
                ostr << '[';
            }


            if( id.m_value_separator.empty() )
                ostr << ' ';
            else {
                ostr << id.m_value_separator;
            }

            value_help( ostr );

            if( optional_value_ )
                ostr << ']';

            ostr << '\n';
        }
    }

    virtual void            help( std::ostream& ostr, cstring negation_prefix_, bool use_color = true )
    {
        usage( ostr, negation_prefix_, use_color );

        if( !p_help.empty() ) {
            ostr << '\n';
            commandline_pretty_print(ostr, "  ", p_help);
        }
    }

protected:
    void                    add_cla_id_impl( cstring prefix,
                                             cstring tag,
                                             cstring value_separator,
                                             bool negatable,
                                             bool validate_value_separator )
    {
        BOOST_TEST_I_ASSRT( !tag.is_empty(),
                            invalid_cla_id() << "Parameter can't have an empty name." );

        BOOST_TEST_I_ASSRT( !prefix.is_empty(),
                            invalid_cla_id() << "Parameter " << tag
                                             << " can't have an empty prefix." );

        BOOST_TEST_I_ASSRT( !value_separator.is_empty(),
                            invalid_cla_id() << "Parameter " << tag
                                             << " can't have an empty value separator." );

        // We trim value separator from all the spaces, so token end will indicate separator
        value_separator.trim();
        BOOST_TEST_I_ASSRT( !validate_value_separator || !value_separator.is_empty() || !p_has_optional_value,
                            invalid_cla_id() << "Parameter " << tag
                                             << " with optional value attribute can't use space as value separator." );

        m_cla_ids.push_back( parameter_cla_id( prefix, tag, value_separator, negatable ) );
    }

private:
    /// interface for usage/help customization
    virtual void            cla_name_help( std::ostream& ostr, cstring cla_tag, cstring /*negation_prefix_*/, bool /*use_color*/ = true) const
    {
        ostr << cla_tag;
    }
    virtual void            value_help( std::ostream& ostr ) const
    {
        if( p_value_hint.empty() )
            ostr << "<value>";
        else
            ostr << p_value_hint;
    }

    // Data members
    param_cla_ids       m_cla_ids;
};

// ************************************************************************** //
// **************              runtime::parameter              ************** //
// ************************************************************************** //

enum args_amount {
    OPTIONAL_PARAM,   // 0-1
    REQUIRED_PARAM,   // exactly 1
    REPEATABLE_PARAM  // 0-N
};

//____________________________________________________________________________//

template<typename ValueType, args_amount a = runtime::OPTIONAL_PARAM, bool is_enum = false>
class parameter : public basic_param {
public:
    /// Constructor with modifiers
#ifndef BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
    template<typename Modifiers=nfp::no_params_type>
    parameter( cstring name, Modifiers const& m = nfp::no_params )
#else
    template<typename Modifiers>
    parameter( cstring name, Modifiers const& m )
#endif
    : basic_param( name, a != runtime::REQUIRED_PARAM, a == runtime::REPEATABLE_PARAM, m )
    , m_arg_factory( m )
    {
        BOOST_TEST_I_ASSRT( !m.has( default_value ) || a == runtime::OPTIONAL_PARAM,
                            invalid_param_spec() << "Parameter " << name
                                                 << " is not optional and can't have default_value." );

        BOOST_TEST_I_ASSRT( !m.has( optional_value ) || !this->p_repeatable,
                            invalid_param_spec() << "Parameter " << name
                                                 << " is repeatable and can't have optional_value." );
    }

private:
    basic_param_ptr clone() const BOOST_OVERRIDE
    {
        return basic_param_ptr( new parameter( *this ) );
    }
    void    produce_argument( cstring token, bool , arguments_store& store ) const BOOST_OVERRIDE
    {
        m_arg_factory.produce_argument( token, this->p_name, store );
    }
    void    produce_default( arguments_store& store ) const BOOST_OVERRIDE
    {
        if( !this->p_has_default_value )
            return;

        m_arg_factory.produce_default( this->p_name, store );
    }

    // Data members
    typedef argument_factory<ValueType, is_enum, a == runtime::REPEATABLE_PARAM> factory_t;
    factory_t       m_arg_factory;
};

//____________________________________________________________________________//

class option : public basic_param {
public:
    /// Constructor with modifiers
#ifndef BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
    template<typename Modifiers=nfp::no_params_type>
    option( cstring name, Modifiers const& m = nfp::no_params )
#else
    template<typename Modifiers>
    option( cstring name, Modifiers const& m )
#endif
    : basic_param( name, true, false, nfp::opt_append( nfp::opt_append( m, optional_value = true), default_value = false) )
    , m_arg_factory( nfp::opt_append( nfp::opt_append( m, optional_value = true), default_value = false) )
    {
    }

    void            add_cla_id( cstring prefix, cstring tag, cstring value_separator, bool negatable = false )
    {
        add_cla_id_impl( prefix, tag, value_separator, negatable, false );
    }

private:
    basic_param_ptr clone() const BOOST_OVERRIDE
    {
        return basic_param_ptr( new option( *this ) );
    }

    void    produce_argument( cstring token, bool negative_form, arguments_store& store ) const BOOST_OVERRIDE
    {
        if( token.empty() )
            store.set( p_name, !negative_form );
        else {
            BOOST_TEST_I_ASSRT( !negative_form,
                                format_error( p_name ) << "Can't set value to negative form of the argument." );

            m_arg_factory.produce_argument( token, p_name, store );
        }
    }

    void    produce_default( arguments_store& store ) const BOOST_OVERRIDE
    {
        m_arg_factory.produce_default( p_name, store );
    }
    void    cla_name_help( std::ostream& ostr, cstring cla_tag, cstring negation_prefix_, bool use_color = true ) const BOOST_OVERRIDE
    {
        namespace utils = unit_test::utils;
        namespace ut_detail = unit_test::ut_detail;

        if( !negation_prefix_.is_empty() ) {
            BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::YELLOW );
            ostr << '[' << negation_prefix_ << ']';
        }
        ostr << cla_tag;
    }
    void    value_help( std::ostream& ostr ) const BOOST_OVERRIDE
    {
        if( p_value_hint.empty() )
            ostr << "<boolean value>";
        else
            ostr << p_value_hint;
    }

    // Data members
    typedef argument_factory<bool, false, false> factory_t;
    factory_t       m_arg_factory;
};

//____________________________________________________________________________//

template<typename EnumType, args_amount a = runtime::OPTIONAL_PARAM>
class enum_parameter : public parameter<EnumType, a, true> {
    typedef parameter<EnumType, a, true> base;
public:
    /// Constructor with modifiers
#ifndef BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
    template<typename Modifiers=nfp::no_params_type>
    enum_parameter( cstring name, Modifiers const& m = nfp::no_params )
#else
    template<typename Modifiers>
    enum_parameter( cstring name, Modifiers const& m )
#endif
    : base( name, m )
    {
#ifdef BOOST_TEST_CLA_NEW_API
        auto const& values = m[enum_values<EnumType>::value];
        auto it = values.begin();
#else
        std::vector<std::pair<cstring, EnumType> > const& values = m[enum_values<EnumType>::value];
        typename std::vector<std::pair<cstring, EnumType> >::const_iterator it = values.begin();
#endif
        while( it != values.end() ) {
            m_valid_names.push_back( it->first );
            ++it;
        }
    }

private:
    basic_param_ptr clone() const BOOST_OVERRIDE
    {
        return basic_param_ptr( new enum_parameter( *this ) );
    }

    void    value_help( std::ostream& ostr ) const BOOST_OVERRIDE
    {
        if( this->p_value_hint.empty() ) {
            ostr << "<";
            bool first = true;
            BOOST_TEST_FOREACH( cstring, name, m_valid_names ) {
                if( first )
                    first = false;
                else
                    ostr << '|';
                ostr << name;
            }
            ostr << ">";
        }
        else
            ostr << this->p_value_hint;
    }

    // Data members
    std::vector<cstring>    m_valid_names;
};


// ************************************************************************** //
// **************           runtime::parameters_store          ************** //
// ************************************************************************** //

class parameters_store {
    struct lg_compare {
        bool operator()( cstring lh, cstring rh ) const
        {
            return std::lexicographical_compare(lh.begin(), lh.end(),
                                                rh.begin(), rh.end());
        }
    };
public:

    typedef std::map<cstring, basic_param_ptr, lg_compare> storage_type;

    /// Adds parameter into the persistent store
    void                    add( basic_param const& in )
    {
        basic_param_ptr p = in.clone();

        BOOST_TEST_I_ASSRT( m_parameters.insert( std::make_pair( cstring(p->p_name), p ) ).second,
                            duplicate_param() << "Parameter " << p->p_name << " is duplicate." );
    }

    /// Returns true if there is no parameters registered
    bool                    is_empty() const    { return m_parameters.empty(); }
    /// Returns map of all the registered parameter
    storage_type const&     all() const         { return m_parameters; }
    /// Returns true if parameter with specified name is registered
    bool                    has( cstring name ) const
    {
        return m_parameters.find( name ) != m_parameters.end();
    }
    /// Returns map of all the registered parameter
    basic_param_ptr         get( cstring name ) const
    {
        storage_type::const_iterator const& found = m_parameters.find( name );
        BOOST_TEST_I_ASSRT( found != m_parameters.end(),
                            unknown_param() << "Parameter " << name << " is unknown." );

        return found->second;
    }

private:
    // Data members
    storage_type            m_parameters;
};

} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP
