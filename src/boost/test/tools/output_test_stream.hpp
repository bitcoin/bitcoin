//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief output_test_stream class definition
// ***************************************************************************

#ifndef BOOST_TEST_OUTPUT_TEST_STREAM_HPP_012705GER
#define BOOST_TEST_OUTPUT_TEST_STREAM_HPP_012705GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/utils/wrap_stringstream.hpp>
#include <boost/test/tools/assertion_result.hpp>

// STL
#include <cstddef>          // for std::size_t

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************               output_test_stream             ************** //
// ************************************************************************** //



namespace boost {
namespace test_tools {

//! Class to be used to simplify testing of ostream-based output operations
class BOOST_TEST_DECL output_test_stream : public wrap_stringstream::wrapped_stream {
    typedef unit_test::const_string const_string;
public:
    //! Constructor
    //!
    //!@param[in] pattern_file_name indicates the name of the file for matching. If the
    //!           string is empty, the standard input or output streams are used instead
    //!           (depending on match_or_save)
    //!@param[in] match_or_save if true, the pattern file will be read, otherwise it will be
    //!           written
    //!@param[in] text_or_binary if false, opens the stream in binary mode. Otherwise the stream
    //!           is opened with default flags and the carriage returns are ignored.
    explicit        output_test_stream( const_string    pattern_file_name = const_string(),
                                        bool            match_or_save     = true,
                                        bool            text_or_binary    = true );

    // Destructor
    ~output_test_stream() BOOST_OVERRIDE;

    //! Checks if the stream is empty
    //!
    //!@param[in] flush_stream if true, flushes the stream after the call
    virtual assertion_result    is_empty( bool flush_stream = true );

    //! Checks the length of the stream
    //!
    //!@param[in] length target length
    //!@param[in] flush_stream if true, flushes the stream after the call. Set to false to call
    //!           additional checks on the same content.
    virtual assertion_result    check_length( std::size_t length, bool flush_stream = true );

    //! Checks the content of the stream against a string
    //!
    //!@param[in] arg_ the target stream
    //!@param[in] flush_stream if true, flushes the stream after the call.
    virtual assertion_result    is_equal( const_string arg_, bool flush_stream = true );

    //! Checks the content of the stream against a pattern file
    //!
    //!@param[in] flush_stream if true, flushes/resets the stream after the call.
    virtual assertion_result    match_pattern( bool flush_stream = true );

    //! Flushes the stream
    void            flush();

protected:

    //! Returns the string representation of the stream
    //!
    //! May be overriden in order to mutate the string before the matching operations.
    virtual std::string get_stream_string_representation() const;

private:
    // helper functions

    //! Length of the stream
    std::size_t     length();

    //! Synching the stream into an internal string representation
    virtual void    sync();

    struct Impl;
    Impl*           m_pimpl;
};

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_OUTPUT_TEST_STREAM_HPP_012705GER
