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
//  Description : supplies offline implementation for the Test Tools
// ***************************************************************************

#ifndef BOOST_TEST_TEST_TOOLS_IPP_012205GER
#define BOOST_TEST_TEST_TOOLS_IPP_012205GER

// Boost.Test
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/tools/context.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <boost/test/tools/detail/fwd.hpp>
#include <boost/test/tools/detail/print_helper.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/execution_monitor.hpp> // execution_aborted

#include <boost/test/detail/throw_exception.hpp>

#include <boost/test/utils/algorithm.hpp>

// Boost
#include <boost/config.hpp>

// STL
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cwchar>
#include <stdexcept>
#include <vector>
#include <utility>
#include <ios>

// !! should we use #include <cstdarg>
#include <stdarg.h>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::strcmp; using ::strlen; using ::isprint; }
#if !defined( BOOST_NO_CWCHAR )
namespace std { using ::wcscmp; }
#endif
# endif


namespace boost {
namespace unit_test {
  // local static variable, needed here for visibility reasons
  lazy_ostream lazy_ostream::inst = lazy_ostream();
}}

namespace boost {
namespace test_tools {
namespace tt_detail {

// ************************************************************************** //
// **************                print_log_value               ************** //
// ************************************************************************** //

void
print_log_value<bool>::operator()( std::ostream& ostr, bool t )
{
     ostr << std::boolalpha << t;
}

void
print_log_value<char>::operator()( std::ostream& ostr, char t )
{
    if( (std::isprint)( static_cast<unsigned char>(t) ) )
        ostr << '\'' << t << '\'';
    else
        ostr << std::hex
#if BOOST_TEST_USE_STD_LOCALE
        << std::showbase
#else
        << "0x"
#endif
        << static_cast<int>(t);
}

//____________________________________________________________________________//

void
print_log_value<unsigned char>::operator()( std::ostream& ostr, unsigned char t )
{
    ostr << std::hex
        // showbase is only available for new style streams:
#if BOOST_TEST_USE_STD_LOCALE
        << std::showbase
#else
        << "0x"
#endif
        << static_cast<int>(t);
}

//____________________________________________________________________________//

void
print_log_value<char const*>::operator()( std::ostream& ostr, char const* t )
{
    ostr << ( t ? t : "null string" );
}

//____________________________________________________________________________//

void
print_log_value<wchar_t const*>::operator()( std::ostream& ostr, wchar_t const* t )
{
    if(t) {
      ostr << static_cast<const void*>(t);
    }
    else {
      ostr << "null w-string";
    }
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            TOOL BOX Implementation           ************** //
// ************************************************************************** //

using ::boost::unit_test::lazy_ostream;

static char const* check_str [] = { " == ", " != ", " < " , " <= ", " > " , " >= " };
static char const* rever_str [] = { " != ", " == ", " >= ", " > " , " <= ", " < "  };

template<typename OutStream>
void
format_report( OutStream& os, assertion_result const& pr, unit_test::lazy_ostream const& assertion_descr,
               tool_level tl, check_type ct,
               std::size_t num_args, va_list args,
               char const*  prefix, char const*  suffix )
{
    using namespace unit_test;

    switch( ct ) {
    case CHECK_PRED:
        os << prefix << assertion_descr << suffix;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;

    case CHECK_BUILT_ASSERTION: {
        os << prefix << assertion_descr << suffix;

        if( tl != PASS ) {
            const_string details_message = pr.message();

            if( !details_message.is_empty() ) {
                os << details_message;
            }
        }
        break;
    }

    case CHECK_MSG:
        if( tl == PASS )
            os << prefix << "'" << assertion_descr << "'" << suffix;
        else
            os << assertion_descr;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;

    case CHECK_EQUAL:
    case CHECK_NE:
    case CHECK_LT:
    case CHECK_LE:
    case CHECK_GT:
    case CHECK_GE: {
        char const*         arg1_descr  = va_arg( args, char const* );
        lazy_ostream const* arg1_val    = va_arg( args, lazy_ostream const* );
        char const*         arg2_descr  = va_arg( args, char const* );
        lazy_ostream const* arg2_val    = va_arg( args, lazy_ostream const* );

        os << prefix << arg1_descr << check_str[ct-CHECK_EQUAL] << arg2_descr << suffix;

        if( tl != PASS )
            os << " [" << *arg1_val << rever_str[ct-CHECK_EQUAL] << *arg2_val << "]" ;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;
    }

    case CHECK_CLOSE:
    case CHECK_CLOSE_FRACTION: {
        char const*         arg1_descr  = va_arg( args, char const* );
        lazy_ostream const* arg1_val    = va_arg( args, lazy_ostream const* );
        char const*         arg2_descr  = va_arg( args, char const* );
        lazy_ostream const* arg2_val    = va_arg( args, lazy_ostream const* );
        /* toler_descr = */               va_arg( args, char const* );
        lazy_ostream const* toler_val   = va_arg( args, lazy_ostream const* );

        os << "difference{" << pr.message()
                            << "} between " << arg1_descr << "{" << *arg1_val
                            << "} and "               << arg2_descr << "{" << *arg2_val
                            << ( tl == PASS ? "} doesn't exceed " : "} exceeds " )
                            << *toler_val;
        if( ct == CHECK_CLOSE )
            os << "%";
        break;
    }
    case CHECK_SMALL: {
        char const*         arg1_descr  = va_arg( args, char const* );
        lazy_ostream const* arg1_val    = va_arg( args, lazy_ostream const* );
        /* toler_descr = */               va_arg( args, char const* );
        lazy_ostream const* toler_val   = va_arg( args, lazy_ostream const* );

        os << "absolute value of " << arg1_descr << "{" << *arg1_val << "}"
                                   << ( tl == PASS ? " doesn't exceed " : " exceeds " )
                                   << *toler_val;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;
    }

    case CHECK_PRED_WITH_ARGS: {
        std::vector< std::pair<char const*, lazy_ostream const*> > args_copy;
        args_copy.reserve( num_args );
        for( std::size_t i = 0; i < num_args; ++i ) {
            char const* desc = va_arg( args, char const* );
            lazy_ostream const* value = va_arg( args, lazy_ostream const* );
            args_copy.push_back( std::make_pair( desc, value ) );
        }

        os << prefix << assertion_descr;

        // print predicate call description
        os << "( ";
        for( std::size_t i = 0; i < num_args; ++i ) {
            os << args_copy[i].first;

            if( i != num_args-1 )
                os << ", ";
        }
        os << " )" << suffix;

        if( tl != PASS ) {
            os << " for ( ";
            for( std::size_t i = 0; i < num_args; ++i ) {
                os << *args_copy[i].second;

                if( i != num_args-1 )
                    os << ", ";
            }
            os << " )";
        }

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;
    }

    case CHECK_EQUAL_COLL: {
        char const* left_begin_descr    = va_arg( args, char const* );
        char const* left_end_descr      = va_arg( args, char const* );
        char const* right_begin_descr   = va_arg( args, char const* );
        char const* right_end_descr     = va_arg( args, char const* );

        os << prefix << "{ " << left_begin_descr  << ", " << left_end_descr  << " } == { "
                             << right_begin_descr << ", " << right_end_descr << " }"
           << suffix;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;
    }

    case CHECK_BITWISE_EQUAL: {
        char const* left_descr    = va_arg( args, char const* );
        char const* right_descr   = va_arg( args, char const* );

        os << prefix << left_descr  << " =.= " << right_descr << suffix;

        if( !pr.has_empty_message() )
            os << ". " << pr.message();
        break;
    }
    }
}

//____________________________________________________________________________//

bool
report_assertion( assertion_result const&   ar,
                  lazy_ostream const&       assertion_descr,
                  const_string              file_name,
                  std::size_t               line_num,
                  tool_level                tl,
                  check_type                ct,
                  std::size_t               num_args, ... )
{
    using namespace unit_test;

    if( !framework::test_in_progress() ) {
        // in case no test is in progress, we do not throw anything:
        // raising an exception here may result in raising an exception in a destructor of a global fixture
        // which will abort the process
        // We flag this as aborted instead

        //BOOST_TEST_I_ASSRT( framework::current_test_case_id() != INV_TEST_UNIT_ID,
        //                    std::runtime_error( "Can't use testing tools outside of test case implementation." ) );

        framework::test_aborted();
        return false;
    }


    if( !!ar )
        tl = PASS;

    log_level    ll;
    char const*  prefix;
    char const*  suffix;

    switch( tl ) {
    case PASS:
        ll      = log_successful_tests;
        prefix  = "check ";
        suffix  = " has passed";
        break;
    case WARN:
        ll      = log_warnings;
        prefix  = "condition ";
        suffix  = " is not satisfied";
        break;
    case CHECK:
        ll      = log_all_errors;
        prefix  = "check ";
        suffix  = " has failed";
        break;
    case REQUIRE:
        ll      = log_fatal_errors;
        prefix  = "critical check ";
        suffix  = " has failed";
        break;
    default:
        return true;
    }

    unit_test_log << unit_test::log::begin( file_name, line_num ) << ll;
    va_list args;
    va_start( args, num_args );

    format_report( unit_test_log, ar, assertion_descr, tl, ct, num_args, args, prefix, suffix );

    va_end( args );
    unit_test_log << unit_test::log::end();

    switch( tl ) {
    case PASS:
        framework::assertion_result( AR_PASSED );
        return true;

    case WARN:
        framework::assertion_result( AR_TRIGGERED );
        return false;

    case CHECK:
        framework::assertion_result( AR_FAILED );
        return false;

    case REQUIRE:
        framework::assertion_result( AR_FAILED );
        framework::test_unit_aborted( framework::current_test_unit() );
        BOOST_TEST_I_THROW( execution_aborted() );
        // the previous line either throws or aborts and the return below is not reached
        // return false;
        BOOST_TEST_UNREACHABLE_RETURN(false);
    }

    return true;
}

//____________________________________________________________________________//

assertion_result
format_assertion_result( const_string expr_val, const_string details )
{
    assertion_result res(false);

    bool starts_new_line = first_char( expr_val ) == '\n';

    if( !starts_new_line && !expr_val.is_empty() )
        res.message().stream() << " [" << expr_val << "]";

    if( !details.is_empty() ) {
        if( first_char(details) != '[' )
            res.message().stream() << ": ";
        else
            res.message().stream() << " ";

        res.message().stream() << details;
    }

    if( starts_new_line )
        res.message().stream() << "." << expr_val;

    return res;
}

//____________________________________________________________________________//

BOOST_TEST_DECL std::string
prod_report_format( assertion_result const& ar, unit_test::lazy_ostream const& assertion_descr, check_type ct, std::size_t num_args, ... )
{
    std::ostringstream msg_buff;

    va_list args;
    va_start( args, num_args );

    format_report( msg_buff, ar, assertion_descr, CHECK, ct, num_args, args, "assertion ", " failed" );

    va_end( args );

    return msg_buff.str();
}

//____________________________________________________________________________//

assertion_result
equal_impl( char const* left, char const* right )
{
    return (left && right) ? std::strcmp( left, right ) == 0 : (left == right);
}

//____________________________________________________________________________//

#if !defined( BOOST_NO_CWCHAR )

assertion_result
equal_impl( wchar_t const* left, wchar_t const* right )
{
    return (left && right) ? std::wcscmp( left, right ) == 0 : (left == right);
}

#endif // !defined( BOOST_NO_CWCHAR )

//____________________________________________________________________________//

bool
is_defined_impl( const_string symbol_name, const_string symbol_value )
{
    symbol_value.trim_left( 2 );
    return symbol_name != symbol_value;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 context_frame                ************** //
// ************************************************************************** //

context_frame::context_frame( ::boost::unit_test::lazy_ostream const& context_descr )
: m_frame_id( unit_test::framework::add_context( context_descr, true ) )
{
}

//____________________________________________________________________________//

context_frame::~context_frame()
{
    unit_test::framework::clear_context( m_frame_id );
}

//____________________________________________________________________________//

context_frame::operator bool()
{
    return true;
}

//____________________________________________________________________________//

} // namespace tt_detail

// ************************************************************************** //
// **************               output_test_stream             ************** //
// ************************************************************************** //

struct output_test_stream::Impl
{
    std::fstream    m_pattern;
    bool            m_match_or_save;
    bool            m_text_or_binary;
    std::string     m_synced_string;

    char            get_char()
    {
        char res = 0;
        do {
            m_pattern.get( res );
        } while( m_text_or_binary && res == '\r' && !m_pattern.fail() && !m_pattern.eof() );

        return res;
    }

    void            check_and_fill( assertion_result& res )
    {
        if( !res.p_predicate_value )
            res.message() << "Output content: \"" << m_synced_string << '\"';
    }
};

//____________________________________________________________________________//

output_test_stream::output_test_stream( const_string pattern_file_name, bool match_or_save, bool text_or_binary )
: m_pimpl( new Impl )
{
    if( !pattern_file_name.is_empty() ) {
        std::ios::openmode m = match_or_save ? std::ios::in : std::ios::out;
        if( !text_or_binary )
            m |= std::ios::binary;

        m_pimpl->m_pattern.open( pattern_file_name.begin(), m );

        if( !m_pimpl->m_pattern.is_open() )
            BOOST_TEST_FRAMEWORK_MESSAGE( "Can't open pattern file " << pattern_file_name << " for " << (match_or_save ? "reading" : "writing") );
    }

    m_pimpl->m_match_or_save    = match_or_save;
    m_pimpl->m_text_or_binary   = text_or_binary;
}

//____________________________________________________________________________//

output_test_stream::~output_test_stream()
{
    delete m_pimpl;
}

//____________________________________________________________________________//

assertion_result
output_test_stream::is_empty( bool flush_stream )
{
    sync();

    assertion_result res( m_pimpl->m_synced_string.empty() );

    m_pimpl->check_and_fill( res );

    if( flush_stream )
        flush();

    return res;
}

//____________________________________________________________________________//

assertion_result
output_test_stream::check_length( std::size_t length_, bool flush_stream )
{
    sync();

    assertion_result res( m_pimpl->m_synced_string.length() == length_ );

    m_pimpl->check_and_fill( res );

    if( flush_stream )
        flush();

    return res;
}

//____________________________________________________________________________//

assertion_result
output_test_stream::is_equal( const_string arg, bool flush_stream )
{
    sync();

    assertion_result res( const_string( m_pimpl->m_synced_string ) == arg );

    m_pimpl->check_and_fill( res );

    if( flush_stream )
        flush();

    return res;
}

//____________________________________________________________________________//

std::string pretty_print_log(std::string str) {

    static const std::string to_replace[] = { "\r", "\n" };
    static const std::string replacement[] = { "\\r", "\\n" };

    return unit_test::utils::replace_all_occurrences_of(
        str,
        to_replace, to_replace + sizeof(to_replace)/sizeof(to_replace[0]),
        replacement, replacement + sizeof(replacement)/sizeof(replacement[0]));
}

assertion_result
output_test_stream::match_pattern( bool flush_stream )
{
    const std::string::size_type n_chars_presuffix = 10;
    sync();

    assertion_result result( true );

    const std::string stream_string_repr = get_stream_string_representation();

    if( !m_pimpl->m_pattern.is_open() ) {
        result = false;
        result.message() << "Pattern file can't be opened!";
    }
    else {
        if( m_pimpl->m_match_or_save ) {

            int offset = 0;
            std::vector<char> last_elements;
            for ( std::string::size_type i = 0; static_cast<int>(i + offset) < static_cast<int>(stream_string_repr.length()); ++i ) {

                char c = m_pimpl->get_char();

                if( last_elements.size() <= n_chars_presuffix ) {
                    last_elements.push_back( c );
                }
                else {
                    last_elements[ i % last_elements.size() ] = c;
                }

                bool is_same = !m_pimpl->m_pattern.fail() &&
                         !m_pimpl->m_pattern.eof()  &&
                         (stream_string_repr[i+offset] == c);

                if( !is_same ) {

                    result = false;

                    std::string::size_type prefix_size  = (std::min)( i + offset, n_chars_presuffix );

                    std::string::size_type suffix_size  = (std::min)( stream_string_repr.length() - i - offset,
                                                                      n_chars_presuffix );

                    // try to log area around the mismatch
                    std::string substr = stream_string_repr.substr(0, i+offset);
                    std::size_t line = std::count(substr.begin(), substr.end(), '\n');
                    std::size_t column = i + offset - substr.rfind('\n');

                    result.message()
                        << "Mismatch at position " << i
                        << " (line " << line
                        << ", column " << column
                        << "): '" << pretty_print_log(std::string(1, stream_string_repr[i+offset])) << "' != '" << pretty_print_log(std::string(1, c)) << "' :\n";

                    // we already escape this substring because we need its actual size for the pretty print
                    // of the difference location.
                    std::string sub_str_prefix(pretty_print_log(stream_string_repr.substr( i + offset - prefix_size, prefix_size )));

                    // we need this substring as is because we compute the best matching substrings on it.
                    std::string sub_str_suffix(stream_string_repr.substr( i + offset, suffix_size));
                    result.message() << "... " << sub_str_prefix + pretty_print_log(sub_str_suffix) << " ..." << '\n';

                    result.message() << "... ";
                    for( std::size_t j = 0; j < last_elements.size() ; j++ )
                        result.message() << pretty_print_log(std::string(1, last_elements[(i + j + 1) % last_elements.size()]));

                    std::vector<char> last_elements_ordered;
                    last_elements_ordered.push_back(c);
                    for( std::string::size_type counter = 0; counter < suffix_size - 1 ; counter++ ) {
                        char c2 = m_pimpl->get_char();

                        if( m_pimpl->m_pattern.fail() || m_pimpl->m_pattern.eof() )
                            break;

                        result.message() << pretty_print_log(std::string(1, c2));

                        last_elements_ordered.push_back(c2);
                    }

                    // tries to find the best substring matching in the remainder of the
                    // two strings
                    std::size_t max_nb_char_in_common = 0;
                    std::size_t best_pattern_start_index = 0;
                    std::size_t best_stream_start_index = 0;
                    for( std::size_t pattern_start_index = best_pattern_start_index;
                         pattern_start_index < last_elements_ordered.size();
                         pattern_start_index++ ) {
                        for( std::size_t stream_start_index = best_stream_start_index;
                             stream_start_index < sub_str_suffix.size();
                             stream_start_index++ ) {

                            std::size_t max_size = (std::min)( last_elements_ordered.size() - pattern_start_index, sub_str_suffix.size() - stream_start_index );
                            if( max_nb_char_in_common > max_size )
                                break; // safely break to go to the outer loop

                            std::size_t nb_char_in_common = 0;
                            for( std::size_t k = 0; k < max_size; k++) {
                                if( last_elements_ordered[pattern_start_index + k] == sub_str_suffix[stream_start_index + k] )
                                    nb_char_in_common ++;
                                else
                                    break; // we take fully matching substring only
                            }

                            if( nb_char_in_common > max_nb_char_in_common ) {
                                max_nb_char_in_common = nb_char_in_common;
                                best_pattern_start_index = pattern_start_index;
                                best_stream_start_index = stream_start_index;
                            }
                        }
                    }

                    // indicates with more precision the location of the mismatchs in "ascii arts" ...
                    result.message() << " ...\n... ";
                    for( std::string::size_type j = 0; j < sub_str_prefix.size(); j++) {
                        result.message() << ' ';
                    }

                    result.message() << '~'; // places the first tilde at the current char that mismatches

                    for( std::size_t k = 1; k < (std::max)(best_pattern_start_index, best_stream_start_index); k++ ) { // 1 is for the current char c
                        std::string s1(pretty_print_log(std::string(1, last_elements_ordered[(std::min)(k, best_pattern_start_index)])));
                        std::string s2(pretty_print_log(std::string(1, sub_str_suffix[(std::min)(k, best_stream_start_index)])));
                        for( int h = static_cast<int>((std::max)(s1.size(), s2.size())); h > 0; h--)
                            result.message() << "~";
                    }

                    if( m_pimpl->m_pattern.eof() ) {
                        result.message() << "    (reference string shorter than current stream)";
                    }

                    result.message() << "\n";

                    // no need to continue if the EOF is reached
                    if( m_pimpl->m_pattern.eof() ) {
                        break;
                    }

                    // first char is a replicat of c, so we do not copy it.
                    for(std::string::size_type counter = 0; counter < last_elements_ordered.size() - 1 ; counter++)
                        last_elements[ (i + 1 + counter) % last_elements.size() ] = last_elements_ordered[counter + 1];

                    i += last_elements_ordered.size()-1;
                    offset += best_stream_start_index - best_pattern_start_index;

                }

            }

            // not needed anymore
            /*
            if(offset > 0 && false) {
                m_pimpl->m_pattern.ignore(
                    static_cast<std::streamsize>( offset ));
            }
            */
        }
        else {
            m_pimpl->m_pattern.write( stream_string_repr.c_str(),
                                      static_cast<std::streamsize>( stream_string_repr.length() ) );
            m_pimpl->m_pattern.flush();
        }
    }

    if( flush_stream )
        flush();

    return result;
}

//____________________________________________________________________________//

void
output_test_stream::flush()
{
    m_pimpl->m_synced_string.erase();

#ifndef BOOST_NO_STRINGSTREAM
    str( std::string() );
#else
    seekp( 0, std::ios::beg );
#endif
}


std::string
output_test_stream::get_stream_string_representation() const {
    return m_pimpl->m_synced_string;
}

//____________________________________________________________________________//

std::size_t
output_test_stream::length()
{
    sync();

    return m_pimpl->m_synced_string.length();
}

//____________________________________________________________________________//

void
output_test_stream::sync()
{
#ifdef BOOST_NO_STRINGSTREAM
    m_pimpl->m_synced_string.assign( str(), pcount() );
    freeze( false );
#else
    m_pimpl->m_synced_string = str();
#endif
}

//____________________________________________________________________________//

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TEST_TOOLS_IPP_012205GER
