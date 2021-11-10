 ///////////////////////////////////////////////////////////////////////////////
/// \file regex_token_iterator.hpp
/// Contains the definition of regex_token_iterator, and STL-compatible iterator
/// for tokenizing a string using a regular expression.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_REGEX_TOKEN_ITERATOR_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_REGEX_TOKEN_ITERATOR_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/xpressive/regex_iterator.hpp>

namespace boost { namespace xpressive { namespace detail
{

//////////////////////////////////////////////////////////////////////////
// regex_token_iterator_impl
//
template<typename BidiIter>
struct regex_token_iterator_impl
  : counted_base<regex_token_iterator_impl<BidiIter> >
{
    typedef sub_match<BidiIter> value_type;

    regex_token_iterator_impl
    (
        BidiIter begin
      , BidiIter cur
      , BidiIter end
      , BidiIter next_search
      , basic_regex<BidiIter> const &rex
      , regex_constants::match_flag_type flags = regex_constants::match_default
      , std::vector<int> subs = std::vector<int>(1, 0)
      , int n = -2
      , bool not_null = false
    )
      : iter_(begin, cur, end, next_search, rex, flags, not_null)
      , result_()
      , n_((-2 == n) ? (int)subs.size() - 1 : n)
      , subs_()
    {
        BOOST_ASSERT(0 != subs.size());
        this->subs_.swap(subs);
    }

    bool next()
    {
        if(-1 != this->n_)
        {
            BidiIter cur = this->iter_.state_.cur_;
            if(0 != (++this->n_ %= (int)this->subs_.size()) || this->iter_.next())
            {
                this->result_ = (-1 == this->subs_[ this->n_ ])
                    ? this->iter_.what_.prefix()
                    : this->iter_.what_[ this->subs_[ this->n_ ] ];
                return true;
            }
            else if(-1 == this->subs_[ this->n_-- ] && cur != this->iter_.state_.end_)
            {
                this->result_ = value_type(cur, this->iter_.state_.end_, true);
                return true;
            }
        }

        return false;
    }

    bool equal_to(regex_token_iterator_impl<BidiIter> const &that) const
    {
        return this->iter_.equal_to(that.iter_) && this->n_ == that.n_;
    }

    regex_iterator_impl<BidiIter> iter_;
    value_type result_;
    int n_;
    std::vector<int> subs_;
};

inline int get_mark_number(int i)
{
    return i;
}

inline std::vector<int> to_vector(int subs)
{
    return std::vector<int>(1, subs);
}

inline std::vector<int> const &to_vector(std::vector<int> const &subs)
{
    return subs;
}

template<typename Int, std::size_t Size>
inline std::vector<int> to_vector(Int const (&sub_matches)[ Size ])
{
    // so that people can specify sub-match indices inline with
    // string literals, like "\1\2\3", leave off the trailing '\0'
    std::size_t const size = Size - is_same<Int, char>::value;
    std::vector<int> vect(size);
    for(std::size_t i = 0; i < size; ++i)
    {
        vect[i] = get_mark_number(sub_matches[i]);
    }
    return vect;
}

template<typename Int>
inline std::vector<int> to_vector(std::vector<Int> const &sub_matches)
{
    BOOST_MPL_ASSERT((is_convertible<Int, int>));
    return std::vector<int>(sub_matches.begin(), sub_matches.end());
}

} // namespace detail

//////////////////////////////////////////////////////////////////////////
// regex_token_iterator
//
template<typename BidiIter>
struct regex_token_iterator
{
    typedef basic_regex<BidiIter> regex_type;
    typedef typename iterator_value<BidiIter>::type char_type;
    typedef sub_match<BidiIter> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type const *pointer;
    typedef value_type const &reference;
    typedef std::forward_iterator_tag iterator_category;

    /// INTERNAL ONLY
    typedef detail::regex_token_iterator_impl<BidiIter> impl_type_;

    /// \post \c *this is the end of sequence iterator.
    regex_token_iterator()
      : impl_()
    {
    }

    /// \param begin The beginning of the character range to search.
    /// \param end The end of the character range to search.
    /// \param rex The regex pattern to search for.
    /// \pre \c [begin,end) is a valid range.
    regex_token_iterator
    (
        BidiIter begin
      , BidiIter end
      , basic_regex<BidiIter> const &rex
    )
      : impl_()
    {
        if(0 != rex.regex_id())
        {
            this->impl_ = new impl_type_(begin, begin, end, begin, rex);
            this->next_();
        }
    }

    /// \param begin The beginning of the character range to search.
    /// \param end The end of the character range to search.
    /// \param rex The regex pattern to search for.
    /// \param args A let() expression with argument bindings for semantic actions.
    /// \pre \c [begin,end) is a valid range.
    template<typename LetExpr>
    regex_token_iterator
    (
        BidiIter begin
      , BidiIter end
      , basic_regex<BidiIter> const &rex
      , detail::let_<LetExpr> const &args
    )
      : impl_()
    {
        if(0 != rex.regex_id())
        {
            this->impl_ = new impl_type_(begin, begin, end, begin, rex);
            detail::bind_args(args, this->impl_->iter_.what_);
            this->next_();
        }
    }

    /// \param begin The beginning of the character range to search.
    /// \param end The end of the character range to search.
    /// \param rex The regex pattern to search for.
    /// \param subs A range of integers designating sub-matches to be treated as tokens.
    /// \param flags Optional match flags, used to control how the expression is matched against the sequence. (See match_flag_type.)
    /// \pre \c [begin,end) is a valid range.
    /// \pre \c subs is either an integer greater or equal to -1,
    ///     or else an array or non-empty \c std::vector\<\> of such integers.
    template<typename Subs>
    regex_token_iterator
    (
        BidiIter begin
      , BidiIter end
      , basic_regex<BidiIter> const &rex
      , Subs const &subs
      , regex_constants::match_flag_type flags = regex_constants::match_default
    )
      : impl_()
    {
        if(0 != rex.regex_id())
        {
            this->impl_ = new impl_type_(begin, begin, end, begin, rex, flags, detail::to_vector(subs));
            this->next_();
        }
    }

    /// \param begin The beginning of the character range to search.
    /// \param end The end of the character range to search.
    /// \param rex The regex pattern to search for.
    /// \param subs A range of integers designating sub-matches to be treated as tokens.
    /// \param args A let() expression with argument bindings for semantic actions.
    /// \param flags Optional match flags, used to control how the expression is matched against the sequence. (See match_flag_type.)
    /// \pre \c [begin,end) is a valid range.
    /// \pre \c subs is either an integer greater or equal to -1,
    ///     or else an array or non-empty \c std::vector\<\> of such integers.
    template<typename Subs, typename LetExpr>
    regex_token_iterator
    (
        BidiIter begin
      , BidiIter end
      , basic_regex<BidiIter> const &rex
      , Subs const &subs
      , detail::let_<LetExpr> const &args
      , regex_constants::match_flag_type flags = regex_constants::match_default
    )
      : impl_()
    {
        if(0 != rex.regex_id())
        {
            this->impl_ = new impl_type_(begin, begin, end, begin, rex, flags, detail::to_vector(subs));
            detail::bind_args(args, this->impl_->iter_.what_);
            this->next_();
        }
    }

    /// \post <tt>*this == that</tt>
    regex_token_iterator(regex_token_iterator<BidiIter> const &that)
      : impl_(that.impl_) // COW
    {
    }

    /// \post <tt>*this == that</tt>
    regex_token_iterator<BidiIter> &operator =(regex_token_iterator<BidiIter> const &that)
    {
        this->impl_ = that.impl_; // COW
        return *this;
    }

    friend bool operator ==(regex_token_iterator<BidiIter> const &left, regex_token_iterator<BidiIter> const &right)
    {
        if(!left.impl_ || !right.impl_)
        {
            return !left.impl_ && !right.impl_;
        }

        return left.impl_->equal_to(*right.impl_);
    }

    friend bool operator !=(regex_token_iterator<BidiIter> const &left, regex_token_iterator<BidiIter> const &right)
    {
        return !(left == right);
    }

    value_type const &operator *() const
    {
        return this->impl_->result_;
    }

    value_type const *operator ->() const
    {
        return &this->impl_->result_;
    }

    /// If N == -1 then sets *this equal to the end of sequence iterator.
    /// Otherwise if N+1 \< subs.size(), then increments N and sets result equal to
    /// ((subs[N] == -1) ? value_type(what.prefix().str()) : value_type(what[subs[N]].str())).
    /// Otherwise if what.prefix().first != what[0].second and if the element match_prev_avail is
    /// not set in flags then sets it. Then locates the next match as if by calling
    /// regex_search(what[0].second, end, what, *pre, flags), with the following variation:
    /// in the event that the previous match found was of zero length (what[0].length() == 0)
    /// then attempts to find a non-zero length match starting at what[0].second, only if that
    /// fails and provided what[0].second != suffix().second does it look for a (possibly zero
    /// length) match starting from what[0].second + 1.  If such a match is found then sets N
    /// equal to zero, and sets result equal to
    /// ((subs[N] == -1) ? value_type(what.prefix().str()) : value_type(what[subs[N]].str())).
    /// Otherwise if no further matches were found, then let last_end be the endpoint of the last
    /// match that was found. Then if last_end != end and subs[0] == -1 sets N equal to -1 and
    /// sets result equal to value_type(last_end, end). Otherwise sets *this equal to the end
    /// of sequence iterator.
    regex_token_iterator<BidiIter> &operator ++()
    {
        this->fork_(); // un-share the implementation
        this->next_();
        return *this;
    }

    regex_token_iterator<BidiIter> operator ++(int)
    {
        regex_token_iterator<BidiIter> tmp(*this);
        ++*this;
        return tmp;
    }

private:

    /// INTERNAL ONLY
    void fork_()
    {
        if(1 != this->impl_->use_count())
        {
            intrusive_ptr<impl_type_> clone = new impl_type_
            (
                this->impl_->iter_.state_.begin_
              , this->impl_->iter_.state_.cur_
              , this->impl_->iter_.state_.end_
              , this->impl_->iter_.state_.next_search_
              , this->impl_->iter_.rex_
              , this->impl_->iter_.flags_
              , this->impl_->subs_
              , this->impl_->n_
              , this->impl_->iter_.not_null_
            );

            // only copy the match_results struct if we have to. Note: if the next call
            // to impl_->next() will return false or call regex_search, we don't need to
            // copy the match_results struct.
            if(-1 != this->impl_->n_ && this->impl_->n_ + 1 != static_cast<int>(this->impl_->subs_.size()))
            {
                // BUGBUG This is expensive -- it causes the sequence_stack to be cleared.
                // Find a better way
                clone->iter_.what_ = this->impl_->iter_.what_;
            }
            else
            {
                // At the very least, copy the action args
                detail::core_access<BidiIter>::get_action_args(clone->iter_.what_)
                    = detail::core_access<BidiIter>::get_action_args(this->impl_->iter_.what_);
            }

            this->impl_.swap(clone);
        }
    }

    /// INTERNAL ONLY
    void next_()
    {
        BOOST_ASSERT(this->impl_ && 1 == this->impl_->use_count());
        if(!this->impl_->next())
        {
            this->impl_ = 0;
        }
    }

    intrusive_ptr<impl_type_> impl_;
};

}} // namespace boost::xpressive

#endif
