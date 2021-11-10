/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatter.hpp
 * \author Andrey Semashev
 * \date   13.07.2012
 *
 * The header contains a formatter function object definition.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_

#include <locale>
#include <ostream>
#include <boost/ref.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/log/detail/sfinae_tools.hpp>
#endif
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/functional/bind_output.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

// This reference class is a workaround for a Boost.Phoenix bug: https://svn.boost.org/trac/boost/ticket/9363
// It is needed to pass output streams by non-const reference to function objects wrapped in phoenix::bind and phoenix::function.
// It's an implementation detail and will be removed when Boost.Phoenix is fixed.
template< typename StreamT >
class stream_ref :
    public reference_wrapper< StreamT >
{
public:
    typedef typename StreamT::char_type char_type;
    typedef typename StreamT::traits_type traits_type;
    typedef typename StreamT::allocator_type allocator_type;
    typedef typename StreamT::streambuf_type streambuf_type;
    typedef typename StreamT::string_type string_type;
    typedef typename StreamT::ostream_type ostream_type;
    typedef typename StreamT::pos_type pos_type;
    typedef typename StreamT::off_type off_type;
    typedef typename StreamT::int_type int_type;

    typedef typename StreamT::failure failure;
    typedef typename StreamT::fmtflags fmtflags;
    typedef typename StreamT::iostate iostate;
    typedef typename StreamT::openmode openmode;
    typedef typename StreamT::seekdir seekdir;
    typedef typename StreamT::Init Init;

    typedef typename StreamT::event event;
    typedef typename StreamT::event_callback event_callback;

    typedef typename StreamT::sentry sentry;

    static BOOST_CONSTEXPR_OR_CONST fmtflags boolalpha = StreamT::boolalpha;
    static BOOST_CONSTEXPR_OR_CONST fmtflags dec = StreamT::dec;
    static BOOST_CONSTEXPR_OR_CONST fmtflags fixed = StreamT::fixed;
    static BOOST_CONSTEXPR_OR_CONST fmtflags hex = StreamT::hex;
    static BOOST_CONSTEXPR_OR_CONST fmtflags internal = StreamT::internal;
    static BOOST_CONSTEXPR_OR_CONST fmtflags left = StreamT::left;
    static BOOST_CONSTEXPR_OR_CONST fmtflags oct = StreamT::oct;
    static BOOST_CONSTEXPR_OR_CONST fmtflags right = StreamT::right;
    static BOOST_CONSTEXPR_OR_CONST fmtflags scientific = StreamT::scientific;
    static BOOST_CONSTEXPR_OR_CONST fmtflags showbase = StreamT::showbase;
    static BOOST_CONSTEXPR_OR_CONST fmtflags showpoint = StreamT::showpoint;
    static BOOST_CONSTEXPR_OR_CONST fmtflags skipws = StreamT::skipws;
    static BOOST_CONSTEXPR_OR_CONST fmtflags unitbuf = StreamT::unitbuf;
    static BOOST_CONSTEXPR_OR_CONST fmtflags uppercase = StreamT::uppercase;
    static BOOST_CONSTEXPR_OR_CONST fmtflags adjustfield = StreamT::adjustfield;
    static BOOST_CONSTEXPR_OR_CONST fmtflags basefield = StreamT::basefield;
    static BOOST_CONSTEXPR_OR_CONST fmtflags floatfield = StreamT::floatfield;

    static BOOST_CONSTEXPR_OR_CONST iostate badbit = StreamT::badbit;
    static BOOST_CONSTEXPR_OR_CONST iostate eofbit = StreamT::eofbit;
    static BOOST_CONSTEXPR_OR_CONST iostate failbit = StreamT::failbit;
    static BOOST_CONSTEXPR_OR_CONST iostate goodbit = StreamT::goodbit;

    static BOOST_CONSTEXPR_OR_CONST openmode app = StreamT::app;
    static BOOST_CONSTEXPR_OR_CONST openmode ate = StreamT::ate;
    static BOOST_CONSTEXPR_OR_CONST openmode binary = StreamT::binary;
    static BOOST_CONSTEXPR_OR_CONST openmode in = StreamT::in;
    static BOOST_CONSTEXPR_OR_CONST openmode out = StreamT::out;
    static BOOST_CONSTEXPR_OR_CONST openmode trunc = StreamT::trunc;

    static BOOST_CONSTEXPR_OR_CONST seekdir beg = StreamT::beg;
    static BOOST_CONSTEXPR_OR_CONST seekdir cur = StreamT::cur;
    static BOOST_CONSTEXPR_OR_CONST seekdir end = StreamT::end;

    static BOOST_CONSTEXPR_OR_CONST event erase_event = StreamT::erase_event;
    static BOOST_CONSTEXPR_OR_CONST event imbue_event = StreamT::imbue_event;
    static BOOST_CONSTEXPR_OR_CONST event copyfmt_event = StreamT::copyfmt_event;


    BOOST_FORCEINLINE explicit stream_ref(StreamT& strm) : reference_wrapper< StreamT >(strm)
    {
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename T >
    BOOST_FORCEINLINE StreamT& operator<< (T&& val) const
    {
        StreamT& strm = this->get();
        strm << static_cast< T&& >(val);
        return strm;
    }
#if defined(BOOST_MSVC) && BOOST_MSVC < 1800
    // MSVC 10 and 11 generate broken code for the perfect forwarding version above if T is an array type (e.g. a string literal)
    template< typename T, unsigned int N >
    BOOST_FORCEINLINE StreamT& operator<< (T (&val)[N]) const
    {
        StreamT& strm = this->get();
        strm << val;
        return strm;
    }
#endif
#else
    template< typename T >
    BOOST_FORCEINLINE StreamT& operator<< (T& val) const
    {
        StreamT& strm = this->get();
        strm << val;
        return strm;
    }

    template< typename T >
    BOOST_FORCEINLINE StreamT& operator<< (T const& val) const
    {
        StreamT& strm = this->get();
        strm << val;
        return strm;
    }
#endif

    BOOST_FORCEINLINE void attach(string_type& str) const { this->get().attach(str); }
    BOOST_FORCEINLINE void detach() const { this->get().detach(); }
    BOOST_FORCEINLINE string_type const& str() const { return this->get().str(); }
    BOOST_FORCEINLINE ostream_type& stream() const { return this->get().stream(); }
    BOOST_FORCEINLINE fmtflags flags() const { return this->get().flags(); }
    BOOST_FORCEINLINE fmtflags flags(fmtflags f) const { return this->get().flags(f); }
    BOOST_FORCEINLINE fmtflags setf(fmtflags f) const { return this->get().setf(f); }
    BOOST_FORCEINLINE fmtflags setf(fmtflags f, fmtflags mask) const { return this->get().setf(f, mask); }
    BOOST_FORCEINLINE void unsetf(fmtflags f) const { this->get().unsetf(f); }

    BOOST_FORCEINLINE std::streamsize precision() const { return this->get().precision(); }
    BOOST_FORCEINLINE std::streamsize precision(std::streamsize p) const { return this->get().precision(p); }

    BOOST_FORCEINLINE std::streamsize width() const { return this->get().width(); }
    BOOST_FORCEINLINE std::streamsize width(std::streamsize w) const { return this->get().width(w); }

    BOOST_FORCEINLINE std::locale getloc() const { return this->get().getloc(); }
    BOOST_FORCEINLINE std::locale imbue(std::locale const& loc) const { return this->get().imbue(loc); }

    static BOOST_FORCEINLINE int xalloc() { return StreamT::xalloc(); }
    BOOST_FORCEINLINE long& iword(int index) const { return this->get().iword(index); }
    BOOST_FORCEINLINE void*& pword(int index) const { return this->get().pword(index); }

    BOOST_FORCEINLINE void register_callback(event_callback fn, int index) const { this->get().register_callback(fn, index); }

    static BOOST_FORCEINLINE bool sync_with_stdio(bool sync = true) { return StreamT::sync_with_stdio(sync); }

    BOOST_EXPLICIT_OPERATOR_BOOL()
    BOOST_FORCEINLINE bool operator! () const { return !this->get(); }

    BOOST_FORCEINLINE iostate rdstate() const { return this->get().rdstate(); }
    BOOST_FORCEINLINE void clear(iostate state = goodbit) const { this->get().clear(state); }
    BOOST_FORCEINLINE void setstate(iostate state) const { this->get().setstate(state); }
    BOOST_FORCEINLINE bool good() const { return this->get().good(); }
    BOOST_FORCEINLINE bool eof() const { return this->get().eof(); }
    BOOST_FORCEINLINE bool fail() const { return this->get().fail(); }
    BOOST_FORCEINLINE bool bad() const { return this->get().bad(); }

    BOOST_FORCEINLINE iostate exceptions() const { return this->get().exceptions(); }
    BOOST_FORCEINLINE void exceptions(iostate s) const { this->get().exceptions(s); }

    BOOST_FORCEINLINE ostream_type* tie() const { return this->get().tie(); }
    BOOST_FORCEINLINE ostream_type* tie(ostream_type* strm) const { return this->get().tie(strm); }

    BOOST_FORCEINLINE streambuf_type* rdbuf() const { return this->get().rdbuf(); }

    BOOST_FORCEINLINE StreamT& copyfmt(std::basic_ios< char_type, traits_type >& rhs) const { return this->get().copyfmt(rhs); }
    BOOST_FORCEINLINE StreamT& copyfmt(StreamT& rhs) const { return this->get().copyfmt(rhs); }

    BOOST_FORCEINLINE char_type fill() const { return this->get().fill(); }
    BOOST_FORCEINLINE char_type fill(char_type ch) const { return this->get().fill(ch); }

    BOOST_FORCEINLINE char narrow(char_type ch, char def) const { return this->get().narrow(ch, def); }
    BOOST_FORCEINLINE char_type widen(char ch) const { return this->get().widen(ch); }

    BOOST_FORCEINLINE StreamT& flush() const { return this->get().flush(); }

    BOOST_FORCEINLINE pos_type tellp() const { return this->get().tellp(); }
    BOOST_FORCEINLINE StreamT& seekp(pos_type pos) const { return this->get().seekp(pos); }
    BOOST_FORCEINLINE StreamT& seekp(off_type off, std::ios_base::seekdir dir) const { return this->get().seekp(off, dir); }

    template< typename CharT >
    BOOST_FORCEINLINE typename boost::log::aux::enable_if_streamable_char_type< CharT, StreamT& >::type
    put(CharT c) const { return this->get().put(c); }

    template< typename CharT >
    BOOST_FORCEINLINE typename boost::log::aux::enable_if_streamable_char_type< CharT, StreamT& >::type
    write(const CharT* p, std::streamsize size) const { return this->get().write(p, size); }
};

template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::boolalpha;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::dec;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::fixed;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::hex;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::internal;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::left;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::oct;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::right;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::scientific;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::showbase;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::showpoint;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::skipws;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::unitbuf;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::uppercase;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::adjustfield;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::basefield;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::fmtflags stream_ref< StreamT >::floatfield;

template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::iostate stream_ref< StreamT >::badbit;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::iostate stream_ref< StreamT >::eofbit;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::iostate stream_ref< StreamT >::failbit;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::iostate stream_ref< StreamT >::goodbit;

template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::app;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::ate;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::binary;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::in;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::out;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::openmode stream_ref< StreamT >::trunc;

template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::seekdir stream_ref< StreamT >::beg;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::seekdir stream_ref< StreamT >::cur;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::seekdir stream_ref< StreamT >::end;

template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::event stream_ref< StreamT >::erase_event;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::event stream_ref< StreamT >::imbue_event;
template< typename StreamT >
BOOST_CONSTEXPR_OR_CONST typename stream_ref< StreamT >::event stream_ref< StreamT >::copyfmt_event;

//! Default log record message formatter
struct message_formatter
{
    typedef void result_type;

    message_formatter() : m_MessageName(expressions::tag::message::get_name())
    {
    }

    template< typename StreamT >
    BOOST_FORCEINLINE result_type operator() (record_view const& rec, StreamT& strm) const
    {
        boost::log::visit< expressions::tag::message::value_type >(m_MessageName, rec, boost::log::bind_output(strm));
    }

private:
    const attribute_name m_MessageName;
};

} // namespace aux

} // namespace expressions

/*!
 * Log record formatter function wrapper.
 */
template< typename CharT >
class basic_formatter
{
    typedef basic_formatter this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    //! Result type
    typedef void result_type;

    //! Character type
    typedef CharT char_type;
    //! Output stream type
    typedef basic_formatting_ostream< char_type > stream_type;

private:
    //! Filter function type
    typedef boost::log::aux::light_function< void (record_view const&, expressions::aux::stream_ref< stream_type >) > formatter_type;

private:
    //! Formatter function
    formatter_type m_Formatter;

public:
    /*!
     * Default constructor. Creates a formatter that only outputs log message.
     */
    basic_formatter() : m_Formatter(expressions::aux::message_formatter())
    {
    }
    /*!
     * Copy constructor
     */
    basic_formatter(basic_formatter const& that) : m_Formatter(that.m_Formatter)
    {
    }
    /*!
     * Move constructor. The moved-from formatter is left in an unspecified state.
     */
    basic_formatter(BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT : m_Formatter(boost::move(that.m_Formatter))
    {
    }

    /*!
     * Initializing constructor. Creates a formatter which will invoke the specified function object.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    basic_formatter(FunT&& fun) : m_Formatter(boost::forward< FunT >(fun))
    {
    }
#elif !defined(BOOST_MSVC) || BOOST_MSVC >= 1600
    template< typename FunT >
    basic_formatter(FunT const& fun, typename boost::disable_if_c< move_detail::is_rv< FunT >::value, boost::log::aux::sfinae_dummy >::type = boost::log::aux::sfinae_dummy()) : m_Formatter(fun)
    {
    }
#else
    // MSVC 9 and older blows up in unexpected ways if we use SFINAE to disable constructor instantiation
    template< typename FunT >
    basic_formatter(FunT const& fun) : m_Formatter(fun)
    {
    }
    template< typename FunT >
    basic_formatter(rv< FunT >& fun) : m_Formatter(fun)
    {
    }
    template< typename FunT >
    basic_formatter(rv< FunT > const& fun) : m_Formatter(static_cast< FunT const& >(fun))
    {
    }
    basic_formatter(rv< this_type > const& that) : m_Formatter(that.m_Formatter)
    {
    }
#endif

    /*!
     * Move assignment. The moved-from formatter is left in an unspecified state.
     */
    basic_formatter& operator= (BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        m_Formatter.swap(that.m_Formatter);
        return *this;
    }
    /*!
     * Copy assignment.
     */
    basic_formatter& operator= (BOOST_COPY_ASSIGN_REF(this_type) that)
    {
        m_Formatter = that.m_Formatter;
        return *this;
    }
    /*!
     * Initializing assignment. Sets the specified function object to the formatter.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    basic_formatter& operator= (FunT&& fun)
    {
        this_type(boost::forward< FunT >(fun)).swap(*this);
        return *this;
    }
#else
    template< typename FunT >
    typename boost::disable_if_c< is_same< typename remove_cv< FunT >::type, this_type >::value, this_type& >::type
    operator= (FunT const& fun)
    {
        this_type(fun).swap(*this);
        return *this;
    }
#endif

    /*!
     * Formatting operator.
     *
     * \param rec A log record to format.
     * \param strm A stream to put the formatted characters to.
     */
    result_type operator() (record_view const& rec, stream_type& strm) const
    {
        m_Formatter(rec, expressions::aux::stream_ref< stream_type >(strm));
    }

    /*!
     * Resets the formatter to the default. The default formatter only outputs message text.
     */
    void reset()
    {
        m_Formatter = expressions::aux::message_formatter();
    }

    /*!
     * Swaps two formatters
     */
    void swap(basic_formatter& that) BOOST_NOEXCEPT
    {
        m_Formatter.swap(that.m_Formatter);
    }
};

template< typename CharT >
inline void swap(basic_formatter< CharT >& left, basic_formatter< CharT >& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

#ifdef BOOST_LOG_USE_CHAR
typedef basic_formatter< char > formatter;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_formatter< wchar_t > wformatter;
#endif

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTER_HPP_INCLUDED_
