///////////////////////////////////////////////////////////////////////////////
/// \file basic_regex.hpp
/// Contains the definition of the basic_regex\<\> class template and its
/// associated helper functions.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_BASIC_REGEX_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_BASIC_REGEX_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_constants.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>
#include <boost/xpressive/detail/core/regex_domain.hpp>

// Doxygen can't handle proto :-(
#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# include <boost/xpressive/detail/static/grammar.hpp>
# include <boost/proto/extends.hpp>
#endif

#if BOOST_XPRESSIVE_HAS_MS_STACK_GUARD
# include <excpt.h>     // for _exception_code()
# include <malloc.h>    // for _resetstkoflw()
#endif

namespace boost { namespace xpressive
{

namespace detail
{
    inline void throw_on_stack_error(bool stack_error)
    {
        BOOST_XPR_ENSURE_(!stack_error, regex_constants::error_stack, "Regex stack space exhausted");
    }
}

///////////////////////////////////////////////////////////////////////////////
// basic_regex
//
/// \brief Class template basic_regex\<\> is a class for holding a compiled regular expression.
template<typename BidiIter>
struct basic_regex
  : proto::extends<
        proto::expr<proto::tag::terminal, proto::term<detail::tracking_ptr<detail::regex_impl<BidiIter> > >, 0>
      , basic_regex<BidiIter>
      , detail::regex_domain
    >
{
private:
    typedef proto::expr<proto::tag::terminal, proto::term<detail::tracking_ptr<detail::regex_impl<BidiIter> > >, 0> pimpl_type;
    typedef proto::extends<pimpl_type, basic_regex<BidiIter>, detail::regex_domain> base_type;

public:
    typedef BidiIter iterator_type;
    typedef typename iterator_value<BidiIter>::type char_type;
    // For compatibility with std::basic_regex
    typedef typename iterator_value<BidiIter>::type value_type;
    typedef typename detail::string_type<char_type>::type string_type;
    typedef regex_constants::syntax_option_type flag_type;

    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, ECMAScript         = regex_constants::ECMAScript);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, icase              = regex_constants::icase_);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, nosubs             = regex_constants::nosubs);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, optimize           = regex_constants::optimize);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, collate            = regex_constants::collate);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, single_line        = regex_constants::single_line);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, not_dot_null       = regex_constants::not_dot_null);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, not_dot_newline    = regex_constants::not_dot_newline);
    BOOST_STATIC_CONSTANT(regex_constants::syntax_option_type, ignore_white_space = regex_constants::ignore_white_space);

    /// \post regex_id()    == 0
    /// \post mark_count()  == 0
    basic_regex()
      : base_type()
    {
    }

    /// \param that The basic_regex object to copy.
    /// \post regex_id()    == that.regex_id()
    /// \post mark_count()  == that.mark_count()
    basic_regex(basic_regex<BidiIter> const &that)
      : base_type(that)
    {
    }

    /// \param that The basic_regex object to copy.
    /// \post regex_id()    == that.regex_id()
    /// \post mark_count()  == that.mark_count()
    /// \return *this
    basic_regex<BidiIter> &operator =(basic_regex<BidiIter> const &that)
    {
        proto::value(*this) = proto::value(that);
        return *this;
    }

    /// Construct from a static regular expression.
    ///
    /// \param  expr The static regular expression
    /// \pre    Expr is the type of a static regular expression.
    /// \post   regex_id()   != 0
    /// \post   mark_count() \>= 0
    template<typename Expr>
    basic_regex(Expr const &expr)
      : base_type()
    {
        BOOST_XPRESSIVE_CHECK_REGEX(Expr, char_type);
        this->compile_(expr, is_valid_regex<Expr, char_type>());
    }

    /// Construct from a static regular expression.
    ///
    /// \param  expr The static regular expression.
    /// \pre    Expr is the type of a static regular expression.
    /// \post   regex_id()   != 0
    /// \post   mark_count() \>= 0
    /// \throw  std::bad_alloc on out of memory
    /// \return *this
    template<typename Expr>
    basic_regex<BidiIter> &operator =(Expr const &expr)
    {
        BOOST_XPRESSIVE_CHECK_REGEX(Expr, char_type);
        this->compile_(expr, is_valid_regex<Expr, char_type>());
        return *this;
    }

    /// Returns the count of capturing sub-expressions in this regular expression
    ///
    std::size_t mark_count() const
    {
        return proto::value(*this) ? proto::value(*this)->mark_count_ : 0;
    }

    /// Returns a token which uniquely identifies this regular expression.
    ///
    regex_id_type regex_id() const
    {
        return proto::value(*this) ? proto::value(*this)->xpr_.get() : 0;
    }

    /// Swaps the contents of this basic_regex object with another.
    ///
    /// \param      that The other basic_regex object.
    /// \attention  This is a shallow swap that does not do reference tracking.
    ///             If you embed a basic_regex object by reference in another
    ///             regular expression and then swap its contents with another
    ///             basic_regex object, the change will not be visible to the
    ///             enclosing regular expression. It is done this way to ensure
    ///             that swap() cannot throw.
    /// \throw      nothrow
    void swap(basic_regex<BidiIter> &that) // throw()
    {
        proto::value(*this).swap(proto::value(that));
    }

    /// Factory method for building a regex object from a range of characters.
    /// Equivalent to regex_compiler\< BidiIter \>().compile(begin, end, flags);
    ///
    /// \param  begin The beginning of a range of characters representing the
    ///         regular expression to compile.
    /// \param  end The end of a range of characters representing the
    ///         regular expression to compile.
    /// \param  flags Optional bitmask that determines how the pat string is
    ///         interpreted. (See syntax_option_type.)
    /// \return A basic_regex object corresponding to the regular expression
    ///         represented by the character range.
    /// \pre    [begin,end) is a valid range.
    /// \pre    The range of characters specified by [begin,end) contains a
    ///         valid string-based representation of a regular expression.
    /// \throw  regex_error when the range of characters has invalid regular
    ///         expression syntax.
    template<typename InputIter>
    static basic_regex<BidiIter> compile(InputIter begin, InputIter end, flag_type flags = regex_constants::ECMAScript)
    {
        return regex_compiler<BidiIter>().compile(begin, end, flags);
    }

    /// \overload
    ///
    template<typename InputRange>
    static basic_regex<BidiIter> compile(InputRange const &pat, flag_type flags = regex_constants::ECMAScript)
    {
        return regex_compiler<BidiIter>().compile(pat, flags);
    }

    /// \overload
    ///
    static basic_regex<BidiIter> compile(char_type const *begin, flag_type flags = regex_constants::ECMAScript)
    {
        return regex_compiler<BidiIter>().compile(begin, flags);
    }

    /// \overload
    ///
    static basic_regex<BidiIter> compile(char_type const *begin, std::size_t len, flag_type flags)
    {
        return regex_compiler<BidiIter>().compile(begin, len, flags);
    }

private:
    friend struct detail::core_access<BidiIter>;

    // Avoid a common programming mistake. Construction from a string is
    // ambiguous. It could mean:
    //   sregex rx = sregex::compile(str); // compile the string into a regex
    // or
    //   sregex rx = as_xpr(str);          // treat the string as a literal
    // Since there is no easy way to disambiguate, it is disallowed. You must
    // say what you mean.

    /// INTERNAL ONLY
    basic_regex(char_type const *);
    /// INTERNAL ONLY
    basic_regex(string_type const &);

    /// INTERNAL ONLY
    bool match_(detail::match_state<BidiIter> &state) const
    {
        #if BOOST_XPRESSIVE_HAS_MS_STACK_GUARD
        bool success = false, stack_error = false;
        __try
        {
            success = proto::value(*this)->xpr_->match(state);
        }
        __except(_exception_code() == 0xC00000FDUL)
        {
            stack_error = true;
            _resetstkoflw();
        }
        detail::throw_on_stack_error(stack_error);
        return success;
        #else
        return proto::value(*this)->xpr_->match(state);
        #endif
    }

    // Compiles valid static regexes into a state machine.
    /// INTERNAL ONLY
    template<typename Expr>
    void compile_(Expr const &expr, mpl::true_)
    {
        detail::static_compile(expr, proto::value(*this).get());
    }

    // No-op for invalid static regexes.
    /// INTERNAL ONLY
    template<typename Expr>
    void compile_(Expr const &, mpl::false_)
    {
    }
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::ECMAScript;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::icase;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::nosubs;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::optimize;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::collate;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::single_line;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::not_dot_null;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::not_dot_newline;
template<typename BidiIter> regex_constants::syntax_option_type const basic_regex<BidiIter>::ignore_white_space;
#endif

///////////////////////////////////////////////////////////////////////////////
// swap
/// \brief      Swaps the contents of two basic_regex objects.
/// \param      left The first basic_regex object.
/// \param      right The second basic_regex object.
/// \attention  This is a shallow swap that does not do reference tracking.
///             If you embed a basic_regex object by reference in another
///             regular expression and then swap its contents with another
///             basic_regex object, the change will not be visible to the
///             enclosing regular expression. It is done this way to ensure
///             that swap() cannot throw.
/// \throw      nothrow
template<typename BidiIter>
inline void swap(basic_regex<BidiIter> &left, basic_regex<BidiIter> &right) // throw()
{
    left.swap(right);
}

}} // namespace boost::xpressive

#endif // BOOST_XPRESSIVE_BASIC_REGEX_HPP_EAN_10_04_2005
