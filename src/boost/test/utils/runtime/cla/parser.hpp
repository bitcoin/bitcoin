//  (C) Copyright Gennadiy Rozental 2001.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief CLA parser
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_CLA_PARSER_HPP
#define BOOST_TEST_UTILS_RUNTIME_CLA_PARSER_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/argument.hpp>
#include <boost/test/utils/runtime/modifier.hpp>
#include <boost/test/utils/runtime/parameter.hpp>

#include <boost/test/utils/runtime/cla/argv_traverser.hpp>

// Boost.Test
#include <boost/test/utils/foreach.hpp>
#include <boost/test/utils/algorithm.hpp>
#include <boost/test/detail/throw_exception.hpp>
#include <boost/test/detail/global_typedef.hpp>

#include <boost/algorithm/cxx11/all_of.hpp> // !! ?? unnecessary after cxx11

// STL
// !! ?? #include <unordered_set>
#include <set>
#include <iostream>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace runtime {
namespace cla {

// ************************************************************************** //
// **************         runtime::cla::parameter_trie         ************** //
// ************************************************************************** //

namespace rt_cla_detail {

struct parameter_trie;
typedef shared_ptr<parameter_trie> parameter_trie_ptr;
typedef std::map<char,parameter_trie_ptr> trie_per_char;
typedef std::vector<boost::reference_wrapper<parameter_cla_id const> > param_cla_id_list;

struct parameter_trie {
    parameter_trie() : m_has_final_candidate( false ) {}

    /// If subtrie corresponding to the char c exists returns it otherwise creates new
    parameter_trie_ptr  make_subtrie( char c )
    {
        trie_per_char::const_iterator it = m_subtrie.find( c );

        if( it == m_subtrie.end() )
            it = m_subtrie.insert( std::make_pair( c, parameter_trie_ptr( new parameter_trie ) ) ).first;

        return it->second;
    }

    /// Creates series of sub-tries per characters in a string
    parameter_trie_ptr  make_subtrie( cstring s )
    {
        parameter_trie_ptr res;

        BOOST_TEST_FOREACH( char, c, s )
            res = (res ? res->make_subtrie( c ) : make_subtrie( c ));

        return res;
    }

    /// Registers candidate parameter for this subtrie. If final, it needs to be unique
    void                add_candidate_id( parameter_cla_id const& param_id, basic_param_ptr param_candidate, bool final )
    {
        BOOST_TEST_I_ASSRT( !m_has_final_candidate && (!final || m_id_candidates.empty()),
          conflicting_param() << "Parameter cla id " << param_id.m_tag << " conflicts with the "
                              << "parameter cla id " << m_id_candidates.back().get().m_tag );

        m_has_final_candidate = final;
        m_id_candidates.push_back( ref(param_id) );

        if( m_id_candidates.size() == 1 )
            m_param_candidate = param_candidate;
        else
            m_param_candidate.reset();
    }

    /// Gets subtrie for specified char if present or nullptr otherwise
    parameter_trie_ptr  get_subtrie( char c ) const
    {
        trie_per_char::const_iterator it = m_subtrie.find( c );

        return it != m_subtrie.end() ? it->second : parameter_trie_ptr();
    }

    // Data members
    trie_per_char       m_subtrie;
    param_cla_id_list   m_id_candidates;
    basic_param_ptr     m_param_candidate;
    bool                m_has_final_candidate;
};

// ************************************************************************** //
// **************      runtime::cla::report_foreing_token      ************** //
// ************************************************************************** //

static void
report_foreing_token( cstring program_name, cstring token )
{
    std::cerr << "Boost.Test WARNING: token \"" << token << "\" does not correspond to the Boost.Test argument \n"
              << "                    and should be placed after all Boost.Test arguments and the -- separator.\n"
              << "                    For example: " << program_name << " --random -- " << token << "\n";
}

} // namespace rt_cla_detail

// ************************************************************************** //
// **************             runtime::cla::parser             ************** //
// ************************************************************************** //

class parser {
public:
    /// Initializes a parser and builds internal trie representation used for
    /// parsing based on the supplied parameters
#ifndef BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
    template<typename Modifiers=nfp::no_params_type>
    parser( parameters_store const& parameters, Modifiers const& m = nfp::no_params )
#else
    template<typename Modifiers>
    parser( parameters_store const& parameters, Modifiers const& m )
#endif
    {
        nfp::opt_assign( m_end_of_param_indicator, m, end_of_params );
        nfp::opt_assign( m_negation_prefix, m, negation_prefix );

        BOOST_TEST_I_ASSRT( algorithm::all_of( m_end_of_param_indicator.begin(),
                                               m_end_of_param_indicator.end(),
                                               parameter_cla_id::valid_prefix_char ),
                            invalid_cla_id() << "End of parameters indicator can only consist of prefix characters." );

        BOOST_TEST_I_ASSRT( algorithm::all_of( m_negation_prefix.begin(),
                                               m_negation_prefix.end(),
                                               parameter_cla_id::valid_name_char ),
                            invalid_cla_id() << "Negation prefix can only consist of prefix characters." );

        build_trie( parameters );
    }

    // input processing method
    int
    parse( int argc, char** argv, runtime::arguments_store& res )
    {
        // save program name for help message
        m_program_name = argv[0];
        cstring path_sep( "\\/" );

        cstring::iterator it = unit_test::utils::find_last_of( m_program_name.begin(), m_program_name.end(),
                                                                path_sep.begin(), path_sep.end() );
        if( it != m_program_name.end() )
            m_program_name.trim_left( it + 1 );

        // Set up the traverser
        argv_traverser tr( argc, (char const**)argv );

        // Loop till we reach end of input
        while( !tr.eoi() ) {
            cstring curr_token = tr.current_token();

            cstring prefix;
            cstring name;
            cstring value_separator;
            bool    negative_form = false;

            // Perform format validations and split the argument into prefix, name and separator
            // False return value indicates end of params indicator is met
            if( !validate_token_format( curr_token, prefix, name, value_separator, negative_form ) ) {
                // get rid of "end of params" token
                tr.next_token();
                break;
            }

            // Locate trie corresponding to found prefix and skip it in the input
            trie_ptr curr_trie = m_param_trie[prefix];

            if( !curr_trie ) {
                //  format_error() << "Unrecognized parameter prefix in the argument " << tr.current_token()
                rt_cla_detail::report_foreing_token( m_program_name, curr_token );
                tr.save_token();
                continue;
            }

            curr_token.trim_left( prefix.size() );

            // Locate parameter based on a name and skip it in the input
            locate_result locate_res = locate_parameter( curr_trie, name, curr_token );
            parameter_cla_id const& found_id    = locate_res.first;
            basic_param_ptr         found_param = locate_res.second;

            if( negative_form ) {
                BOOST_TEST_I_ASSRT( found_id.m_negatable,
                                    format_error( found_param->p_name )
                                        << "Parameter tag " << found_id.m_tag << " is not negatable." );

                curr_token.trim_left( m_negation_prefix.size() );
            }

            curr_token.trim_left( name.size() );

            bool should_go_to_next = true;
            cstring value;


            // Skip validations if parameter has optional value and we are at the end of token
            if( !value_separator.is_empty() || !found_param->p_has_optional_value ) {

                // we are given a separator or there is no optional value

                // Validate and skip value separator in the input
                BOOST_TEST_I_ASSRT( found_id.m_value_separator == value_separator,
                                    format_error( found_param->p_name )
                                        << "Invalid separator for the parameter "
                                        << found_param->p_name
                                        << " in the argument " << tr.current_token() );

                curr_token.trim_left( value_separator.size() );

                // Deduce value source
                value = curr_token;
                if( value.is_empty() ) {
                    tr.next_token();
                    value = tr.current_token();
                }

                BOOST_TEST_I_ASSRT( !value.is_empty(),
                                    format_error( found_param->p_name )
                                        << "Missing an argument value for the parameter "
                                        << found_param->p_name
                                        << " in the argument " << tr.current_token() );
            }
            else if( (value_separator.is_empty() && found_id.m_value_separator.empty()) ) {
                // Deduce value source
                value = curr_token;
                if( value.is_empty() ) {
                    tr.next_token(); // tokenization broke the value, we check the next one

                    if(!found_param->p_has_optional_value) {
                        // there is no separator and there is no optional value
                        // we look for the value on the next token
                        // example "-t XXXX" (no default)
                        // and we commit this value as being the passed value
                        value = tr.current_token();
                    }
                    else {
                        // there is no separator and the value is optional
                        // we check the next token
                        // example "-c" (defaults to true)
                        // and commit this as the value if this is not a token
                        cstring value_check = tr.current_token();

                        cstring prefix_test, name_test, value_separator_test;
                        bool negative_form_test;
                        if( validate_token_format( value_check, prefix_test, name_test, value_separator_test, negative_form_test )
                            && m_param_trie[prefix_test]) {
                          // this is a token, we consume what we have
                          should_go_to_next = false;
                        }
                        else {
                          // this is a value, we commit it
                          value = value_check;
                        }
                    }
                }
            }

            // Validate against argument duplication
            BOOST_TEST_I_ASSRT( !res.has( found_param->p_name ) || found_param->p_repeatable,
                                duplicate_arg( found_param->p_name )
                                    << "Duplicate argument value for the parameter "
                                    << found_param->p_name
                                    << " in the argument " << tr.current_token() );

            // Produce argument value
            found_param->produce_argument( value, negative_form, res );

            if(should_go_to_next) {
                tr.next_token();
            }
        }

        // generate the remainder and return it's size
        return tr.remainder();
    }

    // help/usage/version
    void
    version( std::ostream& ostr )
    {
       ostr << "Boost.Test module ";

#if defined(BOOST_TEST_MODULE)
       // we do not want to refer to the master test suite there
       ostr << '\'' << BOOST_TEST_STRINGIZE( BOOST_TEST_MODULE ).trim( "\"" ) << "' ";
#endif

       ostr << "in executable '" << m_program_name << "'\n";
       ostr << "Compiled from Boost version "
            << BOOST_VERSION/100000      << "."
            << BOOST_VERSION/100 % 1000  << "."
            << BOOST_VERSION % 100       ;
       ostr << " with ";
#if defined(BOOST_TEST_INCLUDED)
       ostr << "header-only inclusion of";
#elif defined(BOOST_TEST_DYN_LINK)
       ostr << "dynamic linking to";
#else
       ostr << "static linking to";
#endif
       ostr << " Boost.Test\n";
       ostr << "- Compiler: " << BOOST_COMPILER << '\n'
            << "- Platform: " << BOOST_PLATFORM << '\n'
            << "- STL     : " << BOOST_STDLIB;
       ostr << std::endl;
    }

    void
    usage(std::ostream& ostr,
          cstring param_name = cstring(),
          bool use_color = true)
    {
        namespace utils = unit_test::utils;
        namespace ut_detail = unit_test::ut_detail;

        if( !param_name.is_empty() ) {
            basic_param_ptr param = locate_parameter( m_param_trie[help_prefix], param_name, "" ).second;
            param->usage( ostr, m_negation_prefix );
        }
        else {
            ostr << "\n  The program '" << m_program_name << "' is a Boost.Test module containing unit tests.";

            {
              BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::ORIGINAL );
              ostr << "\n\n  Usage\n    ";
            }

            {
                BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::GREEN );
                ostr << m_program_name << " [Boost.Test argument]... ";
            }
            if( !m_end_of_param_indicator.empty() ) {
                BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::YELLOW );
                ostr << '[' << m_end_of_param_indicator << " [custom test module argument]...]";
            }
        }

        ostr << "\n\n  Use\n      ";
        {

            BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::GREEN );
            ostr << m_program_name << " --help";
        }
        ostr << "\n  or  ";
        {
            BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::GREEN );
            ostr << m_program_name << " --help=<parameter name>";
        }
        ostr << "\n  for detailed help on Boost.Test parameters.\n";
    }

    void
    help(std::ostream& ostr,
         parameters_store const& parameters,
         cstring param_name,
         bool use_color = true)
    {
        namespace utils = unit_test::utils;
        namespace ut_detail = unit_test::ut_detail;

        if( !param_name.is_empty() ) {
            basic_param_ptr param = locate_parameter( m_param_trie[help_prefix], param_name, "" ).second;
            param->help( ostr, m_negation_prefix, use_color);
            return;
        }

        usage(ostr, cstring(), use_color);

        ostr << "\n\n";
        {
          BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::ORIGINAL );
          ostr << "  Command line flags:\n";
        }
        runtime::commandline_pretty_print(
            ostr,
            "   ",
            "The command line flags of Boost.Test are listed below. "
            "All parameters are optional. You can specify parameter value either "
            "as a command line argument or as a value of its corresponding environment "
            "variable. If a flag is specified as a command line argument and an environment variable "
            "at the same time, the command line takes precedence. "
            "The command line argument "
            "support name guessing, and works with shorter names as long as those are not ambiguous."
        );

        if( !m_end_of_param_indicator.empty() ) {
            ostr << "\n\n   All the arguments after the '";
            {
                BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::YELLOW );
                ostr << m_end_of_param_indicator;
            }
            ostr << "' are ignored by Boost.Test.";
        }


        {
          BOOST_TEST_SCOPE_SETCOLOR( use_color, ostr, term_attr::BRIGHT, term_color::ORIGINAL );
          ostr << "\n\n  Environment variables:\n";
        }
        runtime::commandline_pretty_print(
            ostr,
            "   ",
            "Every argument listed below may also be set by a corresponding environment"
            "variable. For an argument '--argument_x=<value>', the corresponding "
            "environment variable is 'BOOST_TEST_ARGUMENT_X=value"
        );



        ostr << "\n\n  The following parameters are supported:\n";

        BOOST_TEST_FOREACH(
            parameters_store::storage_type::value_type const&,
            v,
            parameters.all() )
        {
            basic_param_ptr param = v.second;
            ostr << "\n";
            param->usage( ostr, m_negation_prefix, use_color);
        }

    }

private:
    typedef rt_cla_detail::parameter_trie_ptr   trie_ptr;
    typedef rt_cla_detail::trie_per_char        trie_per_char;
    typedef std::map<cstring,trie_ptr>          str_to_trie;

    void
    build_trie( parameters_store const& parameters )
    {
        // Iterate over all parameters
        BOOST_TEST_FOREACH( parameters_store::storage_type::value_type const&, v, parameters.all() ) {
            basic_param_ptr param = v.second;

            // Register all parameter's ids in trie.
            BOOST_TEST_FOREACH( parameter_cla_id const&, id, param->cla_ids() ) {
                // This is the trie corresponding to the prefix.
                trie_ptr next_trie = m_param_trie[id.m_prefix];
                if( !next_trie )
                    next_trie = m_param_trie[id.m_prefix] = trie_ptr( new rt_cla_detail::parameter_trie );

                // Build the trie, by following name's characters
                // and register this parameter as candidate on each level
                for( size_t index = 0; index < id.m_tag.size(); ++index ) {
                    next_trie = next_trie->make_subtrie( id.m_tag[index] );

                    next_trie->add_candidate_id( id, param, index == (id.m_tag.size() - 1) );
                }
            }
        }
    }

    bool
    validate_token_format( cstring token, cstring& prefix, cstring& name, cstring& separator, bool& negative_form )
    {
        // Match prefix
        cstring::iterator it = token.begin();
        while( it != token.end() && parameter_cla_id::valid_prefix_char( *it ) )
            ++it;

        prefix.assign( token.begin(), it );

        if( prefix.empty() )
            return true;

        // Match name
        while( it != token.end() && parameter_cla_id::valid_name_char( *it ) )
            ++it;

        name.assign( prefix.end(), it );

        if( name.empty() ) {
            if( prefix == m_end_of_param_indicator )
                return false;

            BOOST_TEST_I_THROW( format_error() << "Invalid format for an actual argument " << token );
        }

        // Match value separator
        while( it != token.end() && parameter_cla_id::valid_separator_char( *it ) )
            ++it;

        separator.assign( name.end(), it );

        // Match negation prefix
        negative_form = !m_negation_prefix.empty() && ( name.substr( 0, m_negation_prefix.size() ) == m_negation_prefix );
        if( negative_form )
            name.trim_left( m_negation_prefix.size() );

        return true;
    }

    // C++03: cannot have references as types
    typedef std::pair<parameter_cla_id, basic_param_ptr> locate_result;

    locate_result
    locate_parameter( trie_ptr curr_trie, cstring name, cstring token )
    {
        std::vector<trie_ptr> typo_candidates;
        std::vector<trie_ptr> next_typo_candidates;
        trie_ptr next_trie;

        BOOST_TEST_FOREACH( char, c, name ) {
            if( curr_trie ) {
                // locate next subtrie corresponding to the char
                next_trie = curr_trie->get_subtrie( c );

                if( next_trie )
                    curr_trie = next_trie;
                else {
                    // Initiate search for typo candicates. We will account for 'wrong char' typo
                    // 'missing char' typo and 'extra char' typo
                    BOOST_TEST_FOREACH( trie_per_char::value_type const&, typo_cand, curr_trie->m_subtrie ) {
                        // 'wrong char' typo
                        typo_candidates.push_back( typo_cand.second );

                        // 'missing char' typo
                        if( (next_trie = typo_cand.second->get_subtrie( c )) )
                            typo_candidates.push_back( next_trie );
                    }

                    // 'extra char' typo
                    typo_candidates.push_back( curr_trie );

                    curr_trie.reset();
                }
            }
            else {
                // go over existing typo candidates and see if they are still viable
                BOOST_TEST_FOREACH( trie_ptr, typo_cand, typo_candidates ) {
                    trie_ptr next_typo_cand = typo_cand->get_subtrie( c );

                    if( next_typo_cand )
                        next_typo_candidates.push_back( next_typo_cand );
                }

                next_typo_candidates.swap( typo_candidates );
                next_typo_candidates.clear();
            }
        }

        if( !curr_trie ) {
            std::vector<cstring> typo_candidate_names;
            std::set<parameter_cla_id const*> unique_typo_candidate; // !! ?? unordered_set
            typo_candidate_names.reserve( typo_candidates.size() );
// !! ??            unique_typo_candidate.reserve( typo_candidates.size() );

            BOOST_TEST_FOREACH( trie_ptr, trie_cand, typo_candidates ) {
                // avoid ambiguos candidate trie
                if( trie_cand->m_id_candidates.size() > 1 )
                    continue;

                BOOST_TEST_FOREACH( parameter_cla_id const&, param_cand, trie_cand->m_id_candidates ) {
                    if( !unique_typo_candidate.insert( &param_cand ).second )
                        continue;

                    typo_candidate_names.push_back( param_cand.m_tag );
                }
            }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            BOOST_TEST_I_THROW( unrecognized_param( std::move(typo_candidate_names) )
                                << "An unrecognized parameter in the argument "
                                << token );
#else
            BOOST_TEST_I_THROW( unrecognized_param( typo_candidate_names )
                                << "An unrecognized parameter in the argument "
                                << token );
#endif
        }

        if( curr_trie->m_id_candidates.size() > 1 ) {
            std::vector<cstring> amb_names;
            BOOST_TEST_FOREACH( parameter_cla_id const&, param_id, curr_trie->m_id_candidates )
                amb_names.push_back( param_id.m_tag );

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            BOOST_TEST_I_THROW( ambiguous_param( std::move( amb_names ) )
                                << "An ambiguous parameter name in the argument " << token );
#else
            BOOST_TEST_I_THROW( ambiguous_param( amb_names )
                                << "An ambiguous parameter name in the argument " << token );
#endif
        }

        return locate_result( curr_trie->m_id_candidates.back().get(), curr_trie->m_param_candidate );
    }

    // Data members
    cstring     m_program_name;
    std::string m_end_of_param_indicator;
    std::string m_negation_prefix;
    str_to_trie m_param_trie;
};

} // namespace cla
} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_CLA_PARSER_HPP
