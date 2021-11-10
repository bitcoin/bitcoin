// Boost token_functions.hpp  ------------------------------------------------//

// Copyright John R. Bandela 2001.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/tokenizer/ for documentation.

// Revision History:
// 01 Oct 2004   Joaquin M Lopez Munoz
//      Workaround for a problem with string::assign in msvc-stlport
// 06 Apr 2004   John Bandela
//      Fixed a bug involving using char_delimiter with a true input iterator
// 28 Nov 2003   Robert Zeh and John Bandela
//      Converted into "fast" functions that avoid using += when
//      the supplied iterator isn't an input_iterator; based on
//      some work done at Archelon and a version that was checked into
//      the boost CVS for a short period of time.
// 20 Feb 2002   John Maddock
//      Removed using namespace std declarations and added
//      workaround for BOOST_NO_STDC_NAMESPACE (the library
//      can be safely mixed with regex).
// 06 Feb 2002   Jeremy Siek
//      Added char_separator.
// 02 Feb 2002   Jeremy Siek
//      Removed tabs and a little cleanup.


#ifndef BOOST_TOKEN_FUNCTIONS_JRB120303_HPP_
#define BOOST_TOKEN_FUNCTIONS_JRB120303_HPP_

#include <vector>
#include <stdexcept>
#include <string>
#include <cctype>
#include <algorithm> // for find_if
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/mpl/if.hpp>
#include <boost/throw_exception.hpp>
#if !defined(BOOST_NO_CWCTYPE)
#include <cwctype>
#endif

//
// the following must not be macros if we are to prefix them
// with std:: (they shouldn't be macros anyway...)
//
#ifdef ispunct
#  undef ispunct
#endif
#ifdef iswpunct
#  undef iswpunct
#endif
#ifdef isspace
#  undef isspace
#endif
#ifdef iswspace
#  undef iswspace
#endif
//
// fix namespace problems:
//
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
 using ::ispunct;
 using ::isspace;
#if !defined(BOOST_NO_CWCTYPE)
 using ::iswpunct;
 using ::iswspace;
#endif
}
#endif

namespace boost{
  //===========================================================================
  // The escaped_list_separator class. Which is a model of TokenizerFunction
  // An escaped list is a super-set of what is commonly known as a comma
  // separated value (csv) list.It is separated into fields by a comma or
  // other character. If the delimiting character is inside quotes, then it is
  // counted as a regular character.To allow for embedded quotes in a field,
  // there can be escape sequences using the \ much like C.
  // The role of the comma, the quotation mark, and the escape
  // character (backslash \), can be assigned to other characters.

  struct escaped_list_error : public std::runtime_error{
    escaped_list_error(const std::string& what_arg):std::runtime_error(what_arg) { }
  };


// The out of the box GCC 2.95 on cygwin does not have a char_traits class.
// MSVC does not like the following typename
  template <class Char,
    class Traits = BOOST_DEDUCED_TYPENAME std::basic_string<Char>::traits_type >
  class escaped_list_separator {

  private:
    typedef std::basic_string<Char,Traits> string_type;
    struct char_eq {
      Char e_;
      char_eq(Char e):e_(e) { }
      bool operator()(Char c) {
        return Traits::eq(e_,c);
      }
    };
    string_type  escape_;
    string_type  c_;
    string_type  quote_;
    bool last_;

    bool is_escape(Char e) {
      char_eq f(e);
      return std::find_if(escape_.begin(),escape_.end(),f)!=escape_.end();
    }
    bool is_c(Char e) {
      char_eq f(e);
      return std::find_if(c_.begin(),c_.end(),f)!=c_.end();
    }
    bool is_quote(Char e) {
      char_eq f(e);
      return std::find_if(quote_.begin(),quote_.end(),f)!=quote_.end();
    }
    template <typename iterator, typename Token>
    void do_escape(iterator& next,iterator end,Token& tok) {
      if (++next == end)
        BOOST_THROW_EXCEPTION(escaped_list_error(std::string("cannot end with escape")));
      if (Traits::eq(*next,'n')) {
        tok+='\n';
        return;
      }
      else if (is_quote(*next)) {
        tok+=*next;
        return;
      }
      else if (is_c(*next)) {
        tok+=*next;
        return;
      }
      else if (is_escape(*next)) {
        tok+=*next;
        return;
      }
      else
        BOOST_THROW_EXCEPTION(escaped_list_error(std::string("unknown escape sequence")));
    }

    public:

    explicit escaped_list_separator(Char  e = '\\',
                                    Char c = ',',Char  q = '\"')
      : escape_(1,e), c_(1,c), quote_(1,q), last_(false) { }

    escaped_list_separator(string_type e, string_type c, string_type q)
      : escape_(e), c_(c), quote_(q), last_(false) { }

    void reset() {last_=false;}

    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next,InputIterator end,Token& tok) {
      bool bInQuote = false;
      tok = Token();

      if (next == end) {
        if (last_) {
          last_ = false;
          return true;
        }
        else
          return false;
      }
      last_ = false;
      for (;next != end;++next) {
        if (is_escape(*next)) {
          do_escape(next,end,tok);
        }
        else if (is_c(*next)) {
          if (!bInQuote) {
            // If we are not in quote, then we are done
            ++next;
            // The last character was a c, that means there is
            // 1 more blank field
            last_ = true;
            return true;
          }
          else tok+=*next;
        }
        else if (is_quote(*next)) {
          bInQuote=!bInQuote;
        }
        else {
          tok += *next;
        }
      }
      return true;
    }
  };

  //===========================================================================
  // The classes here are used by offset_separator and char_separator to implement
  // faster assigning of tokens using assign instead of +=

  namespace tokenizer_detail {
  //===========================================================================
  // Tokenizer was broken for wide character separators, at least on Windows, since
  // CRT functions isspace etc only expect values in [0, 0xFF]. Debug build asserts
  // if higher values are passed in. The traits extension class should take care of this.
  // Assuming that the conditional will always get optimized out in the function
  // implementations, argument types are not a problem since both forms of character classifiers
  // expect an int.

#if !defined(BOOST_NO_CWCTYPE)
  template<typename traits, int N>
  struct traits_extension_details : public traits {
    typedef typename traits::char_type char_type;
    static bool isspace(char_type c)
    {
       return std::iswspace(c) != 0;
    }
    static bool ispunct(char_type c)
    {
       return std::iswpunct(c) != 0;
    }
  };

  template<typename traits>
  struct traits_extension_details<traits, 1> : public traits {
    typedef typename traits::char_type char_type;
    static bool isspace(char_type c)
    {
       return std::isspace(c) != 0;
    }
    static bool ispunct(char_type c)
    {
       return std::ispunct(c) != 0;
    }
  };
#endif


  // In case there is no cwctype header, we implement the checks manually.
  // We make use of the fact that the tested categories should fit in ASCII.
  template<typename traits>
  struct traits_extension : public traits {
    typedef typename traits::char_type char_type;
    static bool isspace(char_type c)
    {
#if !defined(BOOST_NO_CWCTYPE)
      return traits_extension_details<traits, sizeof(char_type)>::isspace(c);
#else
      return static_cast< unsigned >(c) <= 255 && std::isspace(c) != 0;
#endif
    }

    static bool ispunct(char_type c)
    {
#if !defined(BOOST_NO_CWCTYPE)
      return traits_extension_details<traits, sizeof(char_type)>::ispunct(c);
#else
      return static_cast< unsigned >(c) <= 255 && std::ispunct(c) != 0;
#endif
    }
  };

  // The assign_or_plus_equal struct contains functions that implement
  // assign, +=, and clearing based on the iterator type.  The
  // generic case does nothing for plus_equal and clearing, while
  // passing through the call for assign.
  //
  // When an input iterator is being used, the situation is reversed.
  // The assign method does nothing, plus_equal invokes operator +=,
  // and the clearing method sets the supplied token to the default
  // token constructor's result.
  //

  template<class IteratorTag>
  struct assign_or_plus_equal {
    template<class Iterator, class Token>
    static void assign(Iterator b, Iterator e, Token &t) {
      t.assign(b, e);
    }

    template<class Token, class Value>
    static void plus_equal(Token &, const Value &) { }

    // If we are doing an assign, there is no need for the
    // the clear.
    //
    template<class Token>
    static void clear(Token &) { }
  };

  template <>
  struct assign_or_plus_equal<std::input_iterator_tag> {
    template<class Iterator, class Token>
    static void assign(Iterator , Iterator , Token &) { }
    template<class Token, class Value>
    static void plus_equal(Token &t, const Value &v) {
      t += v;
    }
    template<class Token>
    static void clear(Token &t) {
      t = Token();
    }
  };


  template<class Iterator>
  struct pointer_iterator_category{
    typedef std::random_access_iterator_tag type;
  };


  template<class Iterator>
  struct class_iterator_category{
    typedef typename Iterator::iterator_category type;
  };



  // This portably gets the iterator_tag without partial template specialization
  template<class Iterator>
    struct get_iterator_category{
    typedef typename mpl::if_<is_pointer<Iterator>,
      pointer_iterator_category<Iterator>,
      class_iterator_category<Iterator>
    >::type cat;

    typedef typename cat::type iterator_category;
  };


  } // namespace tokenizer_detail


  //===========================================================================
  // The offset_separator class, which is a model of TokenizerFunction.
  // Offset breaks a string into tokens based on a range of offsets

  class offset_separator {
  private:

    std::vector<int> offsets_;
    unsigned int current_offset_;
    bool wrap_offsets_;
    bool return_partial_last_;

  public:
    template <typename Iter>
    offset_separator(Iter begin, Iter end, bool wrap_offsets = true,
                     bool return_partial_last = true)
      : offsets_(begin,end), current_offset_(0),
        wrap_offsets_(wrap_offsets),
        return_partial_last_(return_partial_last) { }

    offset_separator()
      : offsets_(1,1), current_offset_(),
        wrap_offsets_(true), return_partial_last_(true) { }

    void reset() {
      current_offset_ = 0;
    }

    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator end, Token& tok)
    {
      typedef tokenizer_detail::assign_or_plus_equal<
        BOOST_DEDUCED_TYPENAME tokenizer_detail::get_iterator_category<
          InputIterator
        >::iterator_category
      > assigner;

      BOOST_ASSERT(!offsets_.empty());

      assigner::clear(tok);
      InputIterator start(next);

      if (next == end)
        return false;

      if (current_offset_ == offsets_.size())
      {
        if (wrap_offsets_)
          current_offset_=0;
        else
          return false;
      }

      int c = offsets_[current_offset_];
      int i = 0;
      for (; i < c; ++i) {
        if (next == end)break;
        assigner::plus_equal(tok,*next++);
      }
      assigner::assign(start,next,tok);

      if (!return_partial_last_)
        if (i < (c-1) )
          return false;

      ++current_offset_;
      return true;
    }
  };


  //===========================================================================
  // The char_separator class breaks a sequence of characters into
  // tokens based on the character delimiters (very much like bad old
  // strtok). A delimiter character can either be kept or dropped. A
  // kept delimiter shows up as an output token, whereas a dropped
  // delimiter does not.

  // This class replaces the char_delimiters_separator class. The
  // constructor for the char_delimiters_separator class was too
  // confusing and needed to be deprecated. However, because of the
  // default arguments to the constructor, adding the new constructor
  // would cause ambiguity, so instead I deprecated the whole class.
  // The implementation of the class was also simplified considerably.

  enum empty_token_policy { drop_empty_tokens, keep_empty_tokens };

  // The out of the box GCC 2.95 on cygwin does not have a char_traits class.
  template <typename Char,
    typename Tr = BOOST_DEDUCED_TYPENAME std::basic_string<Char>::traits_type >
  class char_separator
  {
    typedef tokenizer_detail::traits_extension<Tr> Traits;
    typedef std::basic_string<Char,Tr> string_type;
  public:
    explicit
    char_separator(const Char* dropped_delims,
                   const Char* kept_delims = 0,
                   empty_token_policy empty_tokens = drop_empty_tokens)
      : m_dropped_delims(dropped_delims),
        m_use_ispunct(false),
        m_use_isspace(false),
        m_empty_tokens(empty_tokens),
        m_output_done(false)
    {
      // Borland workaround
      if (kept_delims)
        m_kept_delims = kept_delims;
    }

                // use ispunct() for kept delimiters and isspace for dropped.
    explicit
    char_separator()
      : m_use_ispunct(true),
        m_use_isspace(true),
        m_empty_tokens(drop_empty_tokens),
        m_output_done(false) { }

    void reset() { }

    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator end, Token& tok)
    {
      typedef tokenizer_detail::assign_or_plus_equal<
        BOOST_DEDUCED_TYPENAME tokenizer_detail::get_iterator_category<
          InputIterator
        >::iterator_category
      > assigner;

      assigner::clear(tok);

      // skip past all dropped_delims
      if (m_empty_tokens == drop_empty_tokens)
        for (; next != end  && is_dropped(*next); ++next)
          { }

      InputIterator start(next);

      if (m_empty_tokens == drop_empty_tokens) {

        if (next == end)
          return false;


        // if we are on a kept_delims move past it and stop
        if (is_kept(*next)) {
          assigner::plus_equal(tok,*next);
          ++next;
        } else
          // append all the non delim characters
          for (; next != end && !is_dropped(*next) && !is_kept(*next); ++next)
            assigner::plus_equal(tok,*next);
      }
      else { // m_empty_tokens == keep_empty_tokens

        // Handle empty token at the end
        if (next == end)
        {
          if (m_output_done == false)
          {
            m_output_done = true;
            assigner::assign(start,next,tok);
            return true;
          }
          else
            return false;
        }

        if (is_kept(*next)) {
          if (m_output_done == false)
            m_output_done = true;
          else {
            assigner::plus_equal(tok,*next);
            ++next;
            m_output_done = false;
          }
        }
        else if (m_output_done == false && is_dropped(*next)) {
          m_output_done = true;
        }
        else {
          if (is_dropped(*next))
            start=++next;
          for (; next != end && !is_dropped(*next) && !is_kept(*next); ++next)
            assigner::plus_equal(tok,*next);
          m_output_done = true;
        }
      }
      assigner::assign(start,next,tok);
      return true;
    }

  private:
    string_type m_kept_delims;
    string_type m_dropped_delims;
    bool m_use_ispunct;
    bool m_use_isspace;
    empty_token_policy m_empty_tokens;
    bool m_output_done;

    bool is_kept(Char E) const
    {
      if (m_kept_delims.length())
        return m_kept_delims.find(E) != string_type::npos;
      else if (m_use_ispunct) {
        return Traits::ispunct(E) != 0;
      } else
        return false;
    }
    bool is_dropped(Char E) const
    {
      if (m_dropped_delims.length())
        return m_dropped_delims.find(E) != string_type::npos;
      else if (m_use_isspace) {
        return Traits::isspace(E) != 0;
      } else
        return false;
    }
  };

  //===========================================================================
  // The following class is DEPRECATED, use class char_separators instead.
  //
  // The char_delimiters_separator class, which is a model of
  // TokenizerFunction.  char_delimiters_separator breaks a string
  // into tokens based on character delimiters. There are 2 types of
  // delimiters. returnable delimiters can be returned as
  // tokens. These are often punctuation. nonreturnable delimiters
  // cannot be returned as tokens. These are often whitespace

  // The out of the box GCC 2.95 on cygwin does not have a char_traits class.
  template <class Char,
    class Tr = BOOST_DEDUCED_TYPENAME std::basic_string<Char>::traits_type >
  class char_delimiters_separator {
  private:

    typedef tokenizer_detail::traits_extension<Tr> Traits;
    typedef std::basic_string<Char,Tr> string_type;
    string_type returnable_;
    string_type nonreturnable_;
    bool return_delims_;
    bool no_ispunct_;
    bool no_isspace_;

    bool is_ret(Char E)const
    {
      if (returnable_.length())
        return  returnable_.find(E) != string_type::npos;
      else{
        if (no_ispunct_) {return false;}
        else{
          int r = Traits::ispunct(E);
          return r != 0;
        }
      }
    }
    bool is_nonret(Char E)const
    {
      if (nonreturnable_.length())
        return  nonreturnable_.find(E) != string_type::npos;
      else{
        if (no_isspace_) {return false;}
        else{
          int r = Traits::isspace(E);
          return r != 0;
        }
      }
    }

  public:
    explicit char_delimiters_separator(bool return_delims = false,
                                       const Char* returnable = 0,
                                       const Char* nonreturnable = 0)
      : returnable_(returnable ? returnable : string_type().c_str()),
        nonreturnable_(nonreturnable ? nonreturnable:string_type().c_str()),
        return_delims_(return_delims), no_ispunct_(returnable!=0),
        no_isspace_(nonreturnable!=0) { }

    void reset() { }

  public:

     template <typename InputIterator, typename Token>
     bool operator()(InputIterator& next, InputIterator end,Token& tok) {
     tok = Token();

     // skip past all nonreturnable delims
     // skip past the returnable only if we are not returning delims
     for (;next!=end && ( is_nonret(*next) || (is_ret(*next)
       && !return_delims_ ) );++next) { }

     if (next == end) {
       return false;
     }

     // if we are to return delims and we are one a returnable one
     // move past it and stop
     if (is_ret(*next) && return_delims_) {
       tok+=*next;
       ++next;
     }
     else
       // append all the non delim characters
       for (;next!=end && !is_nonret(*next) && !is_ret(*next);++next)
         tok+=*next;


     return true;
   }
  };


} //namespace boost

#endif
