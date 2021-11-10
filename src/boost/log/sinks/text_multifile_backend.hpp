/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   text_multifile_backend.hpp
 * \author Andrey Semashev
 * \date   09.06.2009
 *
 * The header contains implementation of a text multi-file sink backend.
 */

#ifndef BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_

#include <ios>
#include <string>
#include <locale>
#include <ostream>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/keywords/auto_newline_mode.hpp>
#include <boost/log/sinks/auto_newline_mode.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

namespace file {

    /*!
     * An adapter class that allows to use regular formatters as file name generators.
     */
    template< typename FormatterT >
    class file_name_composer_adapter
    {
    public:
        //! Functor result type
        typedef filesystem::path result_type;
        //! File name character type
        typedef result_type::string_type::value_type native_char_type;
        //! The adopted formatter type
        typedef FormatterT formatter_type;
        //! Formatting stream type
        typedef basic_formatting_ostream< native_char_type > stream_type;

    private:
        //! The adopted formatter
        formatter_type m_Formatter;
        //! Formatted file name storage
        mutable result_type::string_type m_FileName;
        //! Formatting stream
        mutable stream_type m_FormattingStream;

    public:
        /*!
         * Initializing constructor
         */
        explicit file_name_composer_adapter(formatter_type const& formatter, std::locale const& loc = std::locale()) :
            m_Formatter(formatter),
            m_FormattingStream(m_FileName)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
            m_FormattingStream.imbue(loc);
        }
        /*!
         * Copy constructor
         */
        file_name_composer_adapter(file_name_composer_adapter const& that) :
            m_Formatter(that.m_Formatter),
            m_FormattingStream(m_FileName)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
            m_FormattingStream.imbue(that.m_FormattingStream.getloc());
        }
        /*!
         * Assignment
         */
        file_name_composer_adapter& operator= (file_name_composer_adapter const& that)
        {
            m_Formatter = that.m_Formatter;
            return *this;
        }

        /*!
         * The operator generates a file name based on the log record
         */
        result_type operator() (record_view const& rec) const
        {
            boost::log::aux::cleanup_guard< stream_type > cleanup1(m_FormattingStream);
            boost::log::aux::cleanup_guard< result_type::string_type > cleanup2(m_FileName);

            m_Formatter(rec, m_FormattingStream);
            m_FormattingStream.flush();

            return result_type(m_FileName);
        }
    };

    /*!
     * The function adopts a log record formatter into a file name generator
     *
     * \param fmt The formatter function object to adopt
     * \param loc The locale to use to character code conversion and formatting
     */
    template< typename FormatterT >
    inline file_name_composer_adapter< FormatterT > as_file_name_composer(
        FormatterT const& fmt, std::locale const& loc = std::locale())
    {
        return file_name_composer_adapter< FormatterT >(fmt, loc);
    }

} // namespace file


/*!
 * \brief An implementation of a text multiple files logging sink backend
 *
 * The sink backend puts formatted log records to one of the text files.
 * The particular file is chosen upon each record's attribute values, which allows
 * to distribute records into individual files or to group records related to
 * some entity or process in a separate file.
 */
class text_multifile_backend :
    public basic_formatted_sink_backend< char >
{
    //! Base type
    typedef basic_formatted_sink_backend< char > base_type;

public:
    //! Character type
    typedef base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef base_type::string_type string_type;

    //! File name composer functor type
    typedef boost::log::aux::light_function< filesystem::path (record_view const&) > file_name_composer_type;

private:
    //! \cond

    struct implementation;
    implementation* m_pImpl;

    //! \endcond

public:
    /*!
     * Default constructor. The constructed sink backend has no file name composer and
     * thus will not write any files. All other parameters are set to their defaults.
     */
    BOOST_LOG_API text_multifile_backend();

    /*!
     * Constructor. Creates a sink backend with the specified named parameters.
     * The following named parameters are supported:
     *
     * \li \c auto_newline_mode - Specifies automatic trailing newline insertion mode. Must be a value of
     *                            the \c auto_newline_mode enum. By default, is <tt>auto_newline_mode::insert_if_missing</tt>.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_CALL(text_multifile_backend, construct)
#else
    template< typename... ArgsT >
    explicit text_multifile_backend(ArgsT... const& args);
#endif

    /*!
     * Destructor
     */
    BOOST_LOG_API ~text_multifile_backend();

    /*!
     * The method sets file name composer functional object. Log record formatters are accepted, too.
     *
     * \param composer File name composer functor
     */
    template< typename ComposerT >
    void set_file_name_composer(ComposerT const& composer)
    {
        set_file_name_composer_internal(composer);
    }

    /*!
     * Selects whether a trailing newline should be automatically inserted after every log record. See
     * \c auto_newline_mode description for the possible modes of operation.
     *
     * \param mode The trailing newline insertion mode.
     */
    BOOST_LOG_API void set_auto_newline_mode(auto_newline_mode mode);

    /*!
     * The method writes the message to the sink
     */
    BOOST_LOG_API void consume(record_view const& rec, string_type const& formatted_message);

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Constructor implementation
    template< typename ArgsT >
    void construct(ArgsT const& args)
    {
        construct(args[keywords::auto_newline_mode | insert_if_missing]);
    }
    //! Constructor implementation
    BOOST_LOG_API void construct(auto_newline_mode auto_newline);

    //! The method sets the file name composer
    BOOST_LOG_API void set_file_name_composer_internal(file_name_composer_type const& composer);
#endif // BOOST_LOG_DOXYGEN_PASS
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_
