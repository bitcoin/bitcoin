///////////////////////////////////////////////////////////////////////////////
/// \file match_results.hpp
/// Contains the definition of the match_results type and associated helpers.
/// The match_results type holds the results of a regex_match() or
/// regex_search() operation.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Acknowledgements: Thanks to Markus Schoepflin for helping to track down
// a tricky formatting bug on HP Tru64, and to Steven Watanabe for suggesting
// the fix.

#ifndef BOOST_XPRESSIVE_MATCH_RESULTS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_MATCH_RESULTS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <typeinfo>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/integer.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/optional.hpp>
#include <boost/range/end.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/type_traits/is_function.hpp>
#if BOOST_ITERATOR_ADAPTORS_VERSION >= 0x0200
# include <boost/iterator/filter_iterator.hpp>
#endif
#include <boost/xpressive/regex_constants.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>
#include <boost/xpressive/detail/core/sub_match_vector.hpp>
#include <boost/xpressive/detail/utility/sequence_stack.hpp>
#include <boost/xpressive/detail/core/results_cache.hpp>
#include <boost/xpressive/detail/utility/literals.hpp>
#include <boost/xpressive/detail/utility/algorithm.hpp>
#include <boost/xpressive/detail/utility/counted_base.hpp>
// Doxygen can't handle proto :-(
#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# include <boost/proto/proto_fwd.hpp>
# include <boost/proto/traits.hpp>
#endif

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// type_info_less
//
struct type_info_less
{
    bool operator()(std::type_info const *left, std::type_info const *right) const
    {
        return 0 != left->before(*right);
    }
};

///////////////////////////////////////////////////////////////////////////////
// ActionArgBinding
//
struct ActionArgBinding
  : proto::assign<proto::terminal<action_arg<proto::_, proto::_> >, proto::terminal<proto::_> >
{
};

///////////////////////////////////////////////////////////////////////////////
// results_extras
//
template<typename BidiIter>
struct results_extras
  : counted_base<results_extras<BidiIter> >
{
    sequence_stack<sub_match_impl<BidiIter> > sub_match_stack_;
    results_cache<BidiIter> results_cache_;
};

///////////////////////////////////////////////////////////////////////////////
// char_overflow_handler_
//
struct char_overflow_handler_
{
    void operator ()(numeric::range_check_result result) const // throw(regex_error)
    {
        if(numeric::cInRange != result)
        {
            BOOST_THROW_EXCEPTION(
                regex_error(
                    regex_constants::error_escape
                  , "character escape too large to fit in target character type"
                )
            );
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
// transform_op enum
//
enum transform_op { op_none = 0, op_upper = 1, op_lower = 2 };
enum transform_scope { scope_next = 0, scope_rest = 1 };

///////////////////////////////////////////////////////////////////////////////
// case_converting_iterator
//
template<typename OutputIterator, typename Char>
struct case_converting_iterator
{
    typedef std::output_iterator_tag iterator_category;
    typedef Char value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef case_converting_iterator<OutputIterator, Char> reference;

    case_converting_iterator(OutputIterator const &out, traits<Char> const *tr)
      : out_(out)
      , traits_(tr)
      , next_(op_none)
      , rest_(op_none)
    {}

    OutputIterator base() const
    {
        return this->out_;
    }

    case_converting_iterator &operator ++()
    {
        ++this->out_;
        this->next_ = op_none;
        return *this;
    }

    case_converting_iterator operator ++(int)
    {
        case_converting_iterator tmp(*this);
        ++*this;
        return tmp;
    }

    case_converting_iterator &operator *()
    {
        return *this;
    }

    friend bool set_transform(case_converting_iterator &iter, transform_op trans, transform_scope scope)
    {
        BOOST_ASSERT(scope == scope_next || scope == scope_rest);
        if(scope == scope_next)
            iter.next_ = trans;
        else
            iter.rest_ = trans;
        return true;
    }

    case_converting_iterator &operator =(Char ch)
    {
        switch(this->next_ ? this->next_ : this->rest_)
        {
        case op_lower:
            ch = this->traits_->tolower(ch);
            break;

        case op_upper:
            ch = this->traits_->toupper(ch);
            break;

        default:;
        }

        *this->out_ = ch;
        return *this;
    }

private:
    OutputIterator out_;
    traits<Char> const *traits_;
    transform_op next_, rest_;
};

template<typename Iterator>
inline bool set_transform(Iterator &, transform_op, transform_scope)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// noop_output_iterator
//
template<typename Char>
struct noop_output_iterator
{
    typedef std::output_iterator_tag iterator_category;
    typedef Char value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef noop_output_iterator<Char> reference;

    noop_output_iterator &operator ++()
    {
        return *this;
    }

    noop_output_iterator &operator ++(int)
    {
        return *this;
    }

    noop_output_iterator &operator *()
    {
        return *this;
    }

    noop_output_iterator &operator =(Char const &)
    {
        return *this;
    }
};

struct any_type { any_type(...); };
typedef char no_type;
typedef char (&unary_type)[2];
typedef char (&binary_type)[3];
typedef char (&ternary_type)[4];

no_type check_is_formatter(unary_type, binary_type, ternary_type);

template<typename T>
unary_type check_is_formatter(T const &, binary_type, ternary_type);

template<typename T>
binary_type check_is_formatter(unary_type, T const &, ternary_type);

template<typename T, typename U>
binary_type check_is_formatter(T const &, U const &, ternary_type);

template<typename T>
ternary_type check_is_formatter(unary_type, binary_type, T const &);

template<typename T, typename U>
ternary_type check_is_formatter(T const &, binary_type, U const &);

template<typename T, typename U>
ternary_type check_is_formatter(unary_type, T const &, U const &);

template<typename T, typename U, typename V>
ternary_type check_is_formatter(T const &, U const &, V const &);

struct unary_binary_ternary
{
    typedef unary_type (*unary_fun)(any_type);
    typedef binary_type (*binary_fun)(any_type, any_type);
    typedef ternary_type (*ternary_fun)(any_type, any_type, any_type);
    operator unary_fun();
    operator binary_fun();
    operator ternary_fun();
};

template<typename Formatter, bool IsFunction = is_function<Formatter>::value>
struct formatter_wrapper
  : Formatter
  , unary_binary_ternary
{
    formatter_wrapper();
};

template<typename Formatter>
struct formatter_wrapper<Formatter, true>
  : unary_binary_ternary
{
    operator Formatter *();
};

template<typename Formatter>
struct formatter_wrapper<Formatter *, false>
  : unary_binary_ternary
{
    operator Formatter *();
};

template<typename Formatter, typename What, typename Out, typename Void = void>
struct formatter_arity
{
    static formatter_wrapper<Formatter> &formatter;
    static What &what;
    static Out &out;
    BOOST_STATIC_CONSTANT(
        std::size_t
      , value = sizeof(
            check_is_formatter(
                formatter(what)
              , formatter(what, out)
              , formatter(what, out, regex_constants::format_default)
            )
        ) - 1
    );
    typedef mpl::size_t<value> type;
};

template<typename Formatter, typename What, typename Out>
struct formatter_arity<Formatter, What, Out, typename Formatter::proto_is_expr_>
  : mpl::size_t<4>
{};

template<typename T>
struct is_char_ptr
  : mpl::false_
{};

template<typename T>
struct is_char_ptr<T *>
  : mpl::not_<is_function<T> >
{};

#if BOOST_WORKAROUND(__GNUC__, == 4) && (__GNUC_MINOR__ == 0)
// work around gcc-4.0.1 compiler bug wrt function references
template<typename T>
typename mpl::if_<is_function<T>, T *, T const &>::type
as_callable(T const &t)
{
    return t;
}
#endif

} // detail

///////////////////////////////////////////////////////////////////////////////
// match_results
/// \brief Class template match_results\<\> holds the results of a regex_match() or a
/// regex_search() as a collection of sub_match objects.
///
/// Class template match_results\<\> denotes a collection of sequences representing the result of
/// a regular expression match. Storage for the collection is allocated and freed as necessary by
/// the member functions of class match_results\<\>.
///
/// The class template match_results\<\> conforms to the requirements of a Sequence, as specified
/// in (lib.sequence.reqmts), except that only operations defined for const-qualified Sequences are
/// supported.
template<typename BidiIter>
struct match_results
{
private:
    /// INTERNAL ONLY
    ///
    struct dummy { int i_; };
    typedef int dummy::*bool_type;

public:
    typedef typename iterator_value<BidiIter>::type char_type;
    typedef typename detail::string_type<char_type>::type string_type;
    typedef std::size_t size_type;
    typedef sub_match<BidiIter> value_type;
    typedef typename iterator_difference<BidiIter>::type difference_type;
    typedef value_type const &reference;
    typedef value_type const &const_reference;

    typedef typename detail::sub_match_vector<BidiIter>::iterator iterator;
    typedef typename detail::sub_match_vector<BidiIter>::const_iterator const_iterator;
    typedef typename detail::nested_results<BidiIter> nested_results_type;

    /// \post regex_id() == 0
    /// \post size()     == 0
    /// \post empty()    == true
    /// \post str()      == string_type()
    match_results()
      : regex_id_(0)
      , sub_matches_()
      , base_()
      , prefix_()
      , suffix_()
      , nested_results_()
      , extras_ptr_()
      , traits_()
      , args_()
      , named_marks_()
    {
    }

    /// \param that The match_results object to copy
    /// \post regex_id()  == that.regex_id().
    /// \post size()      == that.size().
    /// \post empty()     == that.empty().
    /// \post str(n)      == that.str(n) for all positive integers n \< that.size().
    /// \post prefix()    == that.prefix().
    /// \post suffix()    == that.suffix().
    /// \post (*this)[n]  == that[n] for all positive integers n \< that.size().
    /// \post length(n)   == that.length(n) for all positive integers n \< that.size().
    /// \post position(n) == that.position(n) for all positive integers n \< that.size().
    match_results(match_results<BidiIter> const &that)
      : regex_id_(that.regex_id_)
      , sub_matches_()
      , base_()
      , prefix_()
      , suffix_()
      , nested_results_()
      , extras_ptr_()
      , traits_()
      , args_(that.args_)
      , named_marks_(that.named_marks_)
    {
        if(that)
        {
            extras_type &extras = this->get_extras_();
            std::size_t size = that.sub_matches_.size();
            detail::sub_match_impl<BidiIter> *sub_matches = extras.sub_match_stack_.push_sequence(size, detail::sub_match_impl<BidiIter>(*that.base_), detail::fill);
            detail::core_access<BidiIter>::init_sub_match_vector(this->sub_matches_, sub_matches, size, that.sub_matches_);

            this->base_ = that.base_;
            this->prefix_ = that.prefix_;
            this->suffix_ = that.suffix_;
            // BUGBUG this doesn't share the extras::sequence_stack
            this->nested_results_ = that.nested_results_;
            this->traits_ = that.traits_;
        }
    }

    ~match_results()
    {
    }

    /// \param that The match_results object to copy.
    /// \post regex_id()  == that.regex_id().
    /// \post size()      == that.size().
    /// \post empty()     == that.empty().
    /// \post str(n)      == that.str(n) for all positive integers n \< that.size().
    /// \post prefix()    == that.prefix().
    /// \post suffix()    == that.suffix().
    /// \post (*this)[n]  == that[n] for all positive integers n \< that.size().
    /// \post length(n)   == that.length(n) for all positive integers n \< that.size().
    /// \post position(n) == that.position(n) for all positive integers n \< that.size().
    match_results<BidiIter> &operator =(match_results<BidiIter> const &that)
    {
        match_results<BidiIter>(that).swap(*this);
        return *this;
    }

    /// Returns one plus the number of marked sub-expressions in the regular
    /// expression that was matched if *this represents the result of a
    /// successful match. Otherwise returns 0.
    size_type size() const
    {
        return this->sub_matches_.size();
    }

    /// Returns size() == 0.
    ///
    bool empty() const
    {
        return 0 == this->size();
    }

    /// Returns (*this)[sub].length().
    ///
    difference_type length(size_type sub = 0) const
    {
        return this->sub_matches_[ sub ].length();
    }

    /// If !(*this)[sub].matched then returns -1. Otherwise returns std::distance(base, (*this)[sub].first),
    /// where base is the start iterator of the sequence that was searched. [Note - unless this is part
    /// of a repeated search with a regex_iterator then base is the same as prefix().first - end note]
    difference_type position(size_type sub = 0) const
    {
        return this->sub_matches_[ sub ].matched ? std::distance(*this->base_, this->sub_matches_[ sub ].first) : -1;
    }

    /// Returns (*this)[sub].str().
    ///
    string_type str(size_type sub = 0) const
    {
        return this->sub_matches_[ sub ].str();
    }

    /// Returns a reference to the sub_match object representing the sequence that
    /// matched marked sub-expression sub. If sub == 0 then returns a reference to
    /// a sub_match object representing the sequence that matched the whole regular
    /// expression. If sub >= size() then returns a sub_match object representing an
    /// unmatched sub-expression.
    template<typename Sub>
    const_reference operator [](Sub const &sub) const
    {
        return this->at_(sub);
    }

    /// Returns a reference to the sub_match object representing the character sequence from
    /// the start of the string being matched/searched, to the start of the match found.
    ///
    /// \pre (*this)[0].matched is true
    const_reference prefix() const
    {
        return this->prefix_ ? *this->prefix_ : this->sub_matches_[this->sub_matches_.size()];
    }

    /// Returns a reference to the sub_match object representing the character sequence from
    /// the end of the match found to the end of the string being matched/searched.
    ///
    /// \pre (*this)[0].matched is true
    const_reference suffix() const
    {
        return this->suffix_ ? *this->suffix_ : this->sub_matches_[this->sub_matches_.size()];
    }

    /// Returns a starting iterator that enumerates over all the marked sub-expression matches
    /// stored in *this.
    ///
    const_iterator begin() const
    {
        return this->sub_matches_.begin();
    }

    /// Returns a terminating iterator that enumerates over all the marked sub-expression
    /// matches stored in *this.
    ///
    const_iterator end() const
    {
        return this->sub_matches_.end();
    }

    /// Returns a true value if (*this)[0].matched, else returns a false value.
    ///
    operator bool_type() const
    {
        return (!this->empty() && this->sub_matches_[ 0 ].matched) ? &dummy::i_ : 0;
    }

    /// Returns true if empty() || !(*this)[0].matched, else returns false.
    ///
    bool operator !() const
    {
        return this->empty() || !this->sub_matches_[ 0 ].matched;
    }

    /// Returns the id of the basic_regex object most recently used with this match_results object.
    ///
    regex_id_type regex_id() const
    {
        return this->regex_id_;
    }

    /// Returns a Sequence of nested match_results elements.
    ///
    nested_results_type const &nested_results() const
    {
        return this->nested_results_;
    }

    /// If \c Format models \c ForwardRange or is a null-terminated string, this function
    /// copies the character sequence in \c fmt to \c OutputIterator \c out. For each format
    /// specifier or escape sequence in \c fmt, replace that sequence with either the character(s) it
    /// represents, or the sequence within <tt>*this</tt> to which it refers. The bitmasks specified in flags
    /// determines what format specifiers or escape sequences are recognized. By default, this is the
    /// format used by ECMA-262, ECMAScript Language Specification, Chapter 15 part 5.4.11 String.prototype.replace.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\>, OutputIterator, regex_constants::match_flag_type\></tt>,
    /// this function returns <tt>fmt(*this, out, flags)</tt>.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\>, OutputIterator\></tt>, this function
    /// returns <tt>fmt(*this, out)</tt>.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\> \></tt>, this function
    /// returns <tt>std::copy(x.begin(), x.end(), out)</tt>, where \c x is the result of
    /// calling <tt>fmt(*this)</tt>.
    template<typename Format, typename OutputIterator>
    OutputIterator format
    (
        OutputIterator out
      , Format const &fmt
      , regex_constants::match_flag_type flags = regex_constants::format_default
      , typename disable_if<detail::is_char_ptr<Format> >::type * = 0
    ) const
    {
        // Is this a formatter object, or a format string?
        typedef
            typename detail::formatter_arity<
                Format
              , match_results<BidiIter>
              , OutputIterator
            >::type
        arity;

        return this->format_(out, fmt, flags, arity());
    }

    /// \overload
    ///
    template<typename OutputIterator>
    OutputIterator format
    (
        OutputIterator out
      , char_type const *fmt
      , regex_constants::match_flag_type flags = regex_constants::format_default
    ) const
    {
        return this->format_(out, boost::as_literal(fmt), flags, mpl::size_t<0>());
    }

    /// If \c Format models \c ForwardRange or is a null-terminated string, this function
    /// returns a copy of the character sequence \c fmt. For each format specifier or escape sequence in \c fmt,
    /// replace that sequence with either the character(s) it represents, or the sequence within
    /// <tt>*this</tt> to which it refers. The bitmasks specified in \c flags determines what format specifiers
    /// or escape sequences are recognized. By default this is the format used by ECMA-262,
    /// ECMAScript Language Specification, Chapter 15 part 5.4.11 String.prototype.replace.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\>, OutputIterator, regex_constants::match_flag_type\></tt>,
    /// this function returns a \c string_type object \c x populated by calling <tt>fmt(*this, out, flags)</tt>,
    /// where \c out is a \c back_insert_iterator into \c x.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\>, OutputIterator\></tt>, this function
    /// returns a \c string_type object \c x populated by calling <tt>fmt(*this, out)</tt>,
    /// where \c out is a \c back_insert_iterator into \c x.
    ///
    /// Otherwise, if \c Format models <tt>Callable\<match_results\<BidiIter\> \></tt>, this function
    /// returns <tt>fmt(*this)</tt>.
    template<typename Format, typename OutputIterator>
    string_type format
    (
        Format const &fmt
      , regex_constants::match_flag_type flags = regex_constants::format_default
      , typename disable_if<detail::is_char_ptr<Format> >::type * = 0
    ) const
    {
        string_type result;
        this->format(std::back_inserter(result), fmt, flags);
        return result;
    }

    /// \overload
    ///
    string_type format
    (
        char_type const *fmt
      , regex_constants::match_flag_type flags = regex_constants::format_default
    ) const
    {
        string_type result;
        this->format(std::back_inserter(result), fmt, flags);
        return result;
    }

    /// Swaps the contents of two match_results objects. Guaranteed not to throw.
    /// \param that The match_results object to swap with.
    /// \post *this contains the sequence of matched sub-expressions that were in that,
    /// that contains the sequence of matched sub-expressions that were in *this.
    /// \throw nothrow
    void swap(match_results<BidiIter> &that) // throw()
    {
        using std::swap;
        swap(this->regex_id_, that.regex_id_);
        this->sub_matches_.swap(that.sub_matches_);
        this->base_.swap(that.base_);
        this->prefix_.swap(that.prefix_);
        this->suffix_.swap(that.suffix_);
        this->nested_results_.swap(that.nested_results_);
        this->extras_ptr_.swap(that.extras_ptr_);
        this->traits_.swap(that.traits_);
        this->args_.swap(that.args_);
    }

    /// TODO document me
    ///
    template<typename Arg>
    match_results<BidiIter> &let(Arg const &arg)
    {
        typedef typename proto::result_of::left<Arg>::type left_type;
        typedef typename proto::result_of::right<Arg>::type right_type;
        typedef typename proto::result_of::value<left_type>::type arg_left_type;
        typedef typename proto::result_of::value<right_type>::type arg_right_type;
        BOOST_MPL_ASSERT((proto::matches<Arg, detail::ActionArgBinding>));
        BOOST_MPL_ASSERT((is_same<typename arg_left_type::type, arg_right_type>));
        this->args_[&typeid(proto::value(proto::left(arg)))] = &proto::value(proto::right(arg));
        return *this;
    }

    /// INTERNAL ONLY
    ///
    match_results<BidiIter> const &operator ()(regex_id_type regex_id, size_type index = 0) const
    {
        // BUGBUG this is linear, make it O(1)
        static match_results<BidiIter> const s_null;

        regex_id_filter_predicate<BidiIter> pred(regex_id);
        typename nested_results_type::const_iterator
            begin = this->nested_results_.begin()
          , end = this->nested_results_.end()
          , cur = detail::find_nth_if(begin, end, index, pred);

        return (cur == end) ? s_null : *cur;
    }

    /// INTERNAL ONLY
    ///
    match_results<BidiIter> const &operator ()(basic_regex<BidiIter> const &rex, std::size_t index = 0) const
    {
        return (*this)(rex.regex_id(), index);
    }

private:

    friend struct detail::core_access<BidiIter>;
    typedef detail::results_extras<BidiIter> extras_type;

    /// INTERNAL ONLY
    ///
    void init_
    (
        regex_id_type regex_id
      , intrusive_ptr<detail::traits<char_type> const> const &tr
      , detail::sub_match_impl<BidiIter> *sub_matches
      , size_type size
      , std::vector<detail::named_mark<char_type> > const &named_marks
    )
    {
        this->traits_ = tr;
        this->regex_id_ = regex_id;
        this->named_marks_ = named_marks;
        detail::core_access<BidiIter>::init_sub_match_vector(this->sub_matches_, sub_matches, size);
    }

    /// INTERNAL ONLY
    ///
    extras_type &get_extras_()
    {
        if(!this->extras_ptr_)
        {
            this->extras_ptr_ = new extras_type;
        }

        return *this->extras_ptr_;
    }

    /// INTERNAL ONLY
    ///
    void set_prefix_suffix_(BidiIter begin, BidiIter end)
    {
        this->base_ = begin;
        this->prefix_ = sub_match<BidiIter>(begin, this->sub_matches_[ 0 ].first, begin != this->sub_matches_[ 0 ].first);
        this->suffix_ = sub_match<BidiIter>(this->sub_matches_[ 0 ].second, end, this->sub_matches_[ 0 ].second != end);

        typename nested_results_type::iterator ibegin = this->nested_results_.begin();
        typename nested_results_type::iterator iend = this->nested_results_.end();
        for( ; ibegin != iend; ++ibegin )
        {
            ibegin->set_prefix_suffix_(begin, end);
        }
    }

    /// INTERNAL ONLY
    ///
    void reset_()
    {
        detail::core_access<BidiIter>::init_sub_match_vector(this->sub_matches_, 0, 0);
    }

    /// INTERNAL ONLY
    ///
    void set_base_(BidiIter base)
    {
        this->base_ = base;

        typename nested_results_type::iterator ibegin = this->nested_results_.begin();
        typename nested_results_type::iterator iend = this->nested_results_.end();
        for( ; ibegin != iend; ++ibegin )
        {
            ibegin->set_base_(base);
        }
    }

    /// INTERNAL ONLY
    ///
    const_reference at_(size_type sub) const
    {
        return this->sub_matches_[ sub ];
    }

    /// INTERNAL ONLY
    ///
    const_reference at_(detail::basic_mark_tag const &mark) const
    {
        return this->sub_matches_[ detail::get_mark_number(mark) ];
    }

    /// INTERNAL ONLY
    ///
    const_reference at_(char_type const *name) const
    {
        for(std::size_t i = 0; i < this->named_marks_.size(); ++i)
        {
            if(this->named_marks_[i].name_ == name)
            {
                return this->sub_matches_[ this->named_marks_[i].mark_nbr_ ];
            }
        }
        BOOST_THROW_EXCEPTION(
            regex_error(regex_constants::error_badmark, "invalid named back-reference")
        );
        // Should never execute, but if it does, this returns
        // a "null" sub_match.
        return this->sub_matches_[this->sub_matches_.size()];
    }

    /// INTERNAL ONLY
    ///
    const_reference at_(string_type const &name) const
    {
        return (*this)[name.c_str()];
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename ForwardRange>
    OutputIterator format2_(OutputIterator out, ForwardRange const &result) const
    {
        return std::copy(boost::begin(result), boost::end(result), out);
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename Char>
    OutputIterator format2_(OutputIterator out, Char const *const &result) const
    {
        Char const *tmp = result;
        BOOST_ASSERT(0 != tmp);
        for(; 0 != *tmp; ++tmp, ++out)
        {
            *out = *tmp;
        }
        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename ForwardRange>
    OutputIterator format_
    (
        OutputIterator out
      , ForwardRange const &format
      , regex_constants::match_flag_type flags
      , mpl::size_t<0>
    ) const
    {
        typedef typename range_const_iterator<ForwardRange>::type iterator;
        iterator cur = boost::begin(format), end = boost::end(format);

        if(0 != (regex_constants::format_literal & flags))
        {
            return std::copy(cur, end, out);
        }
        else if(0 != (regex_constants::format_perl & flags))
        {
            return this->format_perl_(cur, end, out);
        }
        else if(0 != (regex_constants::format_sed & flags))
        {
            return this->format_sed_(cur, end, out);
        }
        else if(0 != (regex_constants::format_all & flags))
        {
            return this->format_all_(cur, end, out);
        }

        return this->format_ecma_262_(cur, end, out);
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename Callable1>
    OutputIterator format_
    (
        OutputIterator out
      , Callable1 const &format
      , regex_constants::match_flag_type
      , mpl::size_t<1>
    ) const
    {
        #if BOOST_WORKAROUND(__GNUC__, == 4) && (__GNUC_MINOR__ == 0)
        return this->format2_(out, detail::as_callable(format)(*this));
        #else
        return this->format2_(out, format(*this));
        #endif
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename Callable2>
    OutputIterator format_
    (
        OutputIterator out
      , Callable2 const &format
      , regex_constants::match_flag_type
      , mpl::size_t<2>
    ) const
    {
        #if BOOST_WORKAROUND(__GNUC__, == 4) && (__GNUC_MINOR__ == 0)
        return detail::as_callable(format)(*this, out);
        #else
        return format(*this, out);
        #endif
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename Callable3>
    OutputIterator format_
    (
        OutputIterator out
      , Callable3 const &format
      , regex_constants::match_flag_type flags
      , mpl::size_t<3>
    ) const
    {
        #if BOOST_WORKAROUND(__GNUC__, == 4) && (__GNUC_MINOR__ == 0)
        return detail::as_callable(format)(*this, out, flags);
        #else
        return format(*this, out, flags);
        #endif
    }

    /// INTERNAL ONLY
    ///
    template<typename OutputIterator, typename Expr>
    OutputIterator format_
    (
        OutputIterator out
      , Expr const &format
      , regex_constants::match_flag_type
      , mpl::size_t<4>
    ) const
    {
        // detail::ReplaceAlgo may be an incomplete type at this point, so
        // we can't construct it directly.
        typedef typename mpl::if_c<true, detail::ReplaceAlgo, OutputIterator>::type ReplaceAlgo;
        return this->format2_(out, ReplaceAlgo()(format, 0, *this));
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_ecma_262_(ForwardIterator cur, ForwardIterator end, OutputIterator out) const
    {
        while(cur != end)
        {
            switch(*cur)
            {
            case BOOST_XPR_CHAR_(char_type, '$'):
                out = this->format_backref_(++cur, end, out);
                break;

            default:
                *out++ = *cur++;
                break;
            }
        }

        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_sed_(ForwardIterator cur, ForwardIterator end, OutputIterator out) const
    {
        while(cur != end)
        {
            switch(*cur)
            {
            case BOOST_XPR_CHAR_(char_type, '&'):
                ++cur;
                out = std::copy(this->sub_matches_[ 0 ].first, this->sub_matches_[ 0 ].second, out);
                break;

            case BOOST_XPR_CHAR_(char_type, '\\'):
                out = this->format_escape_(++cur, end, out);
                break;

            default:
                *out++ = *cur++;
                break;
            }
        }

        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_perl_(ForwardIterator cur, ForwardIterator end, OutputIterator out) const
    {
        detail::case_converting_iterator<OutputIterator, char_type> iout(out, this->traits_.get());

        while(cur != end)
        {
            switch(*cur)
            {
            case BOOST_XPR_CHAR_(char_type, '$'):
                iout = this->format_backref_(++cur, end, iout);
                break;

            case BOOST_XPR_CHAR_(char_type, '\\'):
                if(++cur != end && BOOST_XPR_CHAR_(char_type, 'g') == *cur)
                {
                    iout = this->format_named_backref_(++cur, end, iout);
                }
                else
                {
                    iout = this->format_escape_(cur, end, iout);
                }
                break;

            default:
                *iout++ = *cur++;
                break;
            }
        }

        return iout.base();
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_all_(ForwardIterator cur, ForwardIterator end, OutputIterator out) const
    {
        detail::case_converting_iterator<OutputIterator, char_type> iout(out, this->traits_.get());
        iout = this->format_all_impl_(cur, end, iout);
        BOOST_XPR_ENSURE_(cur == end
          , regex_constants::error_paren, "unbalanced parentheses in format string");
        return iout.base();
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_all_impl_(ForwardIterator &cur, ForwardIterator end, OutputIterator out, bool metacolon = false) const
    {
        int max = 0, sub = 0;
        detail::noop_output_iterator<char_type> noop;

        while(cur != end)
        {
            switch(*cur)
            {
            case BOOST_XPR_CHAR_(char_type, '$'):
                out = this->format_backref_(++cur, end, out);
                break;

            case BOOST_XPR_CHAR_(char_type, '\\'):
                if(++cur != end && BOOST_XPR_CHAR_(char_type, 'g') == *cur)
                {
                    out = this->format_named_backref_(++cur, end, out);
                }
                else
                {
                    out = this->format_escape_(cur, end, out);
                }
                break;

            case BOOST_XPR_CHAR_(char_type, '('):
                out = this->format_all_impl_(++cur, end, out);
                BOOST_XPR_ENSURE_(BOOST_XPR_CHAR_(char_type, ')') == *(cur-1)
                  , regex_constants::error_paren, "unbalanced parentheses in format string");
                break;

            case BOOST_XPR_CHAR_(char_type, '?'):
                BOOST_XPR_ENSURE_(++cur != end
                  , regex_constants::error_subreg, "malformed conditional in format string");
                max = static_cast<int>(this->size() - 1);
                sub = detail::toi(cur, end, *this->traits_, 10, max);
                BOOST_XPR_ENSURE_(0 != sub, regex_constants::error_subreg, "invalid back-reference");
                if(this->sub_matches_[ sub ].matched)
                {
                    out = this->format_all_impl_(cur, end, out, true);
                    if(BOOST_XPR_CHAR_(char_type, ':') == *(cur-1))
                        this->format_all_impl_(cur, end, noop);
                }
                else
                {
                    this->format_all_impl_(cur, end, noop, true);
                    if(BOOST_XPR_CHAR_(char_type, ':') == *(cur-1))
                        out = this->format_all_impl_(cur, end, out);
                }
                return out;

            case BOOST_XPR_CHAR_(char_type, ':'):
                if(metacolon)
                {
                    BOOST_FALLTHROUGH;
            case BOOST_XPR_CHAR_(char_type, ')'):
                    ++cur;
                    return out;
                }
                BOOST_FALLTHROUGH;

            default:
                *out++ = *cur++;
                break;
            }
        }

        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_backref_
    (
        ForwardIterator &cur
      , ForwardIterator end
      , OutputIterator out
    ) const
    {
        if(cur == end)
        {
            *out++ = BOOST_XPR_CHAR_(char_type, '$');
        }
        else if(BOOST_XPR_CHAR_(char_type, '$') == *cur)
        {
            *out++ = *cur++;
        }
        else if(BOOST_XPR_CHAR_(char_type, '&') == *cur) // whole match
        {
            ++cur;
            out = std::copy(this->sub_matches_[ 0 ].first, this->sub_matches_[ 0 ].second, out);
        }
        else if(BOOST_XPR_CHAR_(char_type, '`') == *cur) // prefix
        {
            ++cur;
            out = std::copy(this->prefix().first, this->prefix().second, out);
        }
        else if(BOOST_XPR_CHAR_(char_type, '\'') == *cur) // suffix
        {
            ++cur;
            out = std::copy(this->suffix().first, this->suffix().second, out);
        }
        else if(-1 != this->traits_->value(*cur, 10)) // a sub-match
        {
            int max = static_cast<int>(this->size() - 1);
            int sub = detail::toi(cur, end, *this->traits_, 10, max);
            BOOST_XPR_ENSURE_(0 != sub, regex_constants::error_subreg, "invalid back-reference");
            if(this->sub_matches_[ sub ].matched)
                out = std::copy(this->sub_matches_[ sub ].first, this->sub_matches_[ sub ].second, out);
        }
        else
        {
            *out++ = BOOST_XPR_CHAR_(char_type, '$');
            *out++ = *cur++;
        }

        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_escape_
    (
        ForwardIterator &cur
      , ForwardIterator end
      , OutputIterator out
    ) const
    {
        using namespace regex_constants;
        ForwardIterator tmp;
        // define an unsigned type the same size as char_type
        typedef typename boost::uint_t<CHAR_BIT * sizeof(char_type)>::least uchar_t;
        BOOST_MPL_ASSERT_RELATION(sizeof(uchar_t), ==, sizeof(char_type));
        typedef numeric::conversion_traits<uchar_t, int> converstion_traits;
        numeric::converter<int, uchar_t, converstion_traits, detail::char_overflow_handler_> converter;

        if(cur == end)
        {
            *out++ = BOOST_XPR_CHAR_(char_type, '\\');
            return out;
        }

        char_type ch = *cur++;
        switch(ch)
        {
        case BOOST_XPR_CHAR_(char_type, 'a'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\a');
            break;

        case BOOST_XPR_CHAR_(char_type, 'e'):
            *out++ = converter(27);
            break;

        case BOOST_XPR_CHAR_(char_type, 'f'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\f');
            break;

        case BOOST_XPR_CHAR_(char_type, 'n'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\n');
            break;

        case BOOST_XPR_CHAR_(char_type, 'r'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\r');
            break;

        case BOOST_XPR_CHAR_(char_type, 't'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\t');
            break;

        case BOOST_XPR_CHAR_(char_type, 'v'):
            *out++ = BOOST_XPR_CHAR_(char_type, '\v');
            break;

        case BOOST_XPR_CHAR_(char_type, 'x'):
            BOOST_XPR_ENSURE_(cur != end, error_escape, "unexpected end of format found");
            if(BOOST_XPR_CHAR_(char_type, '{') == *cur)
            {
                BOOST_XPR_ENSURE_(++cur != end, error_escape, "unexpected end of format found");
                tmp = cur;
                *out++ = converter(detail::toi(cur, end, *this->traits_, 16, 0xffff));
                BOOST_XPR_ENSURE_(4 == std::distance(tmp, cur) && cur != end && BOOST_XPR_CHAR_(char_type, '}') == *cur++
                  , error_escape, "invalid hex escape : must be \\x { HexDigit HexDigit HexDigit HexDigit }");
            }
            else
            {
                tmp = cur;
                *out++ = converter(detail::toi(cur, end, *this->traits_, 16, 0xff));
                BOOST_XPR_ENSURE_(2 == std::distance(tmp, cur), error_escape
                  , "invalid hex escape : must be \\x HexDigit HexDigit");
            }
            break;

        case BOOST_XPR_CHAR_(char_type, 'c'):
            BOOST_XPR_ENSURE_(cur != end, error_escape, "unexpected end of format found");
            BOOST_XPR_ENSURE_
            (
                this->traits_->in_range(BOOST_XPR_CHAR_(char_type, 'a'), BOOST_XPR_CHAR_(char_type, 'z'), *cur)
             || this->traits_->in_range(BOOST_XPR_CHAR_(char_type, 'A'), BOOST_XPR_CHAR_(char_type, 'Z'), *cur)
              , error_escape
              , "invalid escape control letter; must be one of a-z or A-Z"
            );
            // Convert to character according to ECMA-262, section 15.10.2.10:
            *out++ = converter(*cur % 32);
            ++cur;
            break;

        case BOOST_XPR_CHAR_(char_type, 'l'):
            if(!set_transform(out, detail::op_lower, detail::scope_next))
            {
                *out++ = BOOST_XPR_CHAR_(char_type, 'l');
            }
            break;

        case BOOST_XPR_CHAR_(char_type, 'L'):
            if(!set_transform(out, detail::op_lower, detail::scope_rest))
            {
                *out++ = BOOST_XPR_CHAR_(char_type, 'L');
            }
            break;

        case BOOST_XPR_CHAR_(char_type, 'u'):
            if(!set_transform(out, detail::op_upper, detail::scope_next))
            {
                *out++ = BOOST_XPR_CHAR_(char_type, 'u');
            }
            break;

        case BOOST_XPR_CHAR_(char_type, 'U'):
            if(!set_transform(out, detail::op_upper, detail::scope_rest))
            {
                *out++ = BOOST_XPR_CHAR_(char_type, 'U');
            }
            break;

        case BOOST_XPR_CHAR_(char_type, 'E'):
            if(!set_transform(out, detail::op_none, detail::scope_rest))
            {
                *out++ = BOOST_XPR_CHAR_(char_type, 'E');
            }
            break;

        default:
            // BUGBUG what about backreferences like \12 ?
            if(0 < this->traits_->value(ch, 10))
            {
                int sub = this->traits_->value(ch, 10);
                if(this->sub_matches_[ sub ].matched)
                    out = std::copy(this->sub_matches_[ sub ].first, this->sub_matches_[ sub ].second, out);
            }
            else
            {
                *out++ = ch;
            }
            break;
        }

        return out;
    }

    /// INTERNAL ONLY
    ///
    template<typename ForwardIterator, typename OutputIterator>
    OutputIterator format_named_backref_
    (
        ForwardIterator &cur
      , ForwardIterator end
      , OutputIterator out
    ) const
    {
        using namespace regex_constants;
        BOOST_XPR_ENSURE_(cur != end && BOOST_XPR_CHAR_(char_type, '<') == *cur++
            , error_badmark, "invalid named back-reference");
        ForwardIterator begin = cur;
        for(; cur != end && BOOST_XPR_CHAR_(char_type, '>') != *cur; ++cur)
        {}
        BOOST_XPR_ENSURE_(cur != begin && cur != end && BOOST_XPR_CHAR_(char_type, '>') == *cur
            , error_badmark, "invalid named back-reference");

        string_type name(begin, cur++);
        for(std::size_t i = 0; i < this->named_marks_.size(); ++i)
        {
            if(this->named_marks_[i].name_ == name)
            {
                std::size_t sub = this->named_marks_[i].mark_nbr_;
                return std::copy(this->sub_matches_[ sub ].first, this->sub_matches_[ sub ].second, out);
            }
        }

        BOOST_THROW_EXCEPTION(regex_error(error_badmark, "invalid named back-reference"));
        // Should never get here
        return out;
    }

    regex_id_type regex_id_;
    detail::sub_match_vector<BidiIter> sub_matches_;
    boost::optional<BidiIter> base_;
    boost::optional<sub_match<BidiIter> > prefix_;
    boost::optional<sub_match<BidiIter> > suffix_;
    nested_results_type nested_results_;
    intrusive_ptr<extras_type> extras_ptr_;
    intrusive_ptr<detail::traits<char_type> const> traits_;
    detail::action_args_type args_;
    std::vector<detail::named_mark<char_type> > named_marks_;
};

///////////////////////////////////////////////////////////////////////////////
// regex_id_filter_predicate
//
template<typename BidiIter>
struct regex_id_filter_predicate
{
    typedef match_results<BidiIter> argument_type;
    typedef bool result_type;

    regex_id_filter_predicate(regex_id_type regex_id)
      : regex_id_(regex_id)
    {
    }

    bool operator ()(match_results<BidiIter> const &res) const
    {
        return this->regex_id_ == res.regex_id();
    }

private:

    regex_id_type regex_id_;
};

}} // namespace boost::xpressive

#ifdef BOOST_HAS_CONCEPTS
// Better living through concepts. :-P
namespace std
{
    template<typename Iter_, typename Char_>
    concept_map OutputIterator<
        boost::xpressive::detail::case_converting_iterator<Iter_, Char_>
      , Char_
    >
    {};

    template<typename Char_>
    concept_map OutputIterator<
        boost::xpressive::detail::noop_output_iterator<Char_>
      , Char_
    >
    {};
}
#endif

#endif
