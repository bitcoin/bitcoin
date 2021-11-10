// Three-state boolean logic library

// Copyright Douglas Gregor 2002-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_LOGIC_TRIBOOL_IO_HPP
#define BOOST_LOGIC_TRIBOOL_IO_HPP

#include <boost/logic/tribool.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/noncopyable.hpp>

#if defined(_MSC_VER)
#  pragma once
#endif

#ifndef BOOST_NO_STD_LOCALE
#  include <locale>
#endif

#include <string>
#include <iostream>

namespace boost { namespace logic {

#ifdef BOOST_NO_STD_LOCALE

/**
 * \brief Returns a string containing the default name for the \c
 * false value of a tribool with the given character type T.
 *
 * This function only exists when the C++ standard library
 * implementation does not support locales.
 */
template<typename T> std::basic_string<T> default_false_name();

/**
 * \brief Returns the character string "false".
 *
 * This function only exists when the C++ standard library
 * implementation does not support locales.
 */
template<>
inline std::basic_string<char> default_false_name<char>()
{ return "false"; }

#  if !defined(BOOST_NO_CWCHAR)
/**
 * \brief Returns the wide character string L"false".
 *
 * This function only exists when the C++ standard library
 * implementation does not support locales.
 */
template<>
inline std::basic_string<wchar_t> default_false_name<wchar_t>()
{ return L"false"; }
#  endif

/**
 * \brief Returns a string containing the default name for the \c true
 * value of a tribool with the given character type T.
 *
 * This function only exists when the C++ standard library
 * implementation does not support locales.
 */
template<typename T> std::basic_string<T> default_true_name();

/**
 * \brief Returns the character string "true".
 *
 * This function only exists when the C++ standard library
 * implementation does not support locales.
 */
template<>
inline std::basic_string<char> default_true_name<char>()
{ return "true"; }

#  if !defined(BOOST_NO_CWCHAR)
/**
 * \brief Returns the wide character string L"true".
 *
 *  This function only exists * when the C++ standard library
 *  implementation does not support * locales.
 */
template<>
inline std::basic_string<wchar_t> default_true_name<wchar_t>()
{ return L"true"; }
#  endif
#endif

/**
 * \brief Returns a string containing the default name for the indeterminate
 * value of a tribool with the given character type T.
 *
 * This routine is used by the input and output streaming operators
 * for tribool when there is no locale support or the stream's locale
 * does not contain the indeterminate_name facet.
 */
template<typename T> std::basic_string<T> get_default_indeterminate_name();

/// Returns the character string "indeterminate".
template<>
inline std::basic_string<char> get_default_indeterminate_name<char>()
{ return "indeterminate"; }

#if !defined(BOOST_NO_CWCHAR)
/// Returns the wide character string L"indeterminate".
template<>
inline std::basic_string<wchar_t> get_default_indeterminate_name<wchar_t>()
{ return L"indeterminate"; }
#endif

// http://www.cantrip.org/locale.html

#ifndef BOOST_NO_STD_LOCALE
/**
 * \brief A locale facet specifying the name of the indeterminate
 * value of a tribool.
 *
 * The facet is used to perform I/O on tribool values when \c
 * std::boolalpha has been specified. This class template is only
 * available if the C++ standard library implementation supports
 * locales.
 */
template<typename CharT>
class indeterminate_name : public std::locale::facet, private boost::noncopyable
{
public:
  typedef CharT char_type;
  typedef std::basic_string<CharT> string_type;

  /// Construct the facet with the default name
  indeterminate_name() : name_(get_default_indeterminate_name<CharT>()) {}

  /// Construct the facet with the given name for the indeterminate value
  explicit indeterminate_name(const string_type& initial_name)
  : name_(initial_name) {}

  /// Returns the name for the indeterminate value
  string_type name() const { return name_; }

  /// Uniquily identifies this facet with the locale.
  static std::locale::id id;

private:
  string_type name_;
};

template<typename CharT> std::locale::id indeterminate_name<CharT>::id;
#endif

/**
 * \brief Writes the value of a tribool to a stream.
 *
 * When the value of @p x is either \c true or \c false, this routine
 * is semantically equivalent to:
 * \code out << static_cast<bool>(x); \endcode
 *
 * When @p x has an indeterminate value, it outputs either the integer
 * value 2 (if <tt>(out.flags() & std::ios_base::boolalpha) == 0</tt>)
 * or the name of the indeterminate value. The name of the
 * indeterminate value comes from the indeterminate_name facet (if it
 * is defined in the output stream's locale), or from the
 * get_default_indeterminate_name function (if it is not defined in the
 * locale or if the C++ standard library implementation does not
 * support locales).
 *
 * \returns @p out
 */
template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& out, tribool x)
{
  if (!indeterminate(x)) {
    out << static_cast<bool>(x);
  } else {
    typename std::basic_ostream<CharT, Traits>::sentry cerberus(out);
    if (cerberus) {
      if (out.flags() & std::ios_base::boolalpha) {
#ifndef BOOST_NO_STD_LOCALE
        if (BOOST_HAS_FACET(indeterminate_name<CharT>, out.getloc())) {
          const indeterminate_name<CharT>& facet =
            BOOST_USE_FACET(indeterminate_name<CharT>, out.getloc());
          out << facet.name();
        } else {
          out << get_default_indeterminate_name<CharT>();
        }
#else
        out << get_default_indeterminate_name<CharT>();
#endif
      }
      else
        out << 2;
    }
  }
  return out;
}

/**
 * \brief Writes the indeterminate tribool value to a stream.
 *
 * This routine outputs either the integer
 * value 2 (if <tt>(out.flags() & std::ios_base::boolalpha) == 0</tt>)
 * or the name of the indeterminate value. The name of the
 * indeterminate value comes from the indeterminate_name facet (if it
 * is defined in the output stream's locale), or from the
 * get_default_indeterminate_name function (if it is not defined in the
 * locale or if the C++ standard library implementation does not
 * support locales).
 *
 * \returns @p out
 */
template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& out, 
           bool (*)(tribool, detail::indeterminate_t))
{ return out << tribool(indeterminate); } 

/**
 * \brief Reads a tribool value from a stream.
 *
 * When <tt>(out.flags() & std::ios_base::boolalpha) == 0</tt>, this
 * function reads a \c long value from the input stream @p in and
 * converts that value to a tribool. If that value is 0, @p x becomes
 * \c false; if it is 1, @p x becomes \c true; if it is 2, @p becomes
 * \c indetermine; otherwise, the operation fails (and the fail bit is
 * set on the input stream @p in).
 *
 * When <tt>(out.flags() & std::ios_base::boolalpha) != 0</tt>, this
 * function first determines the names of the false, true, and
 * indeterminate values. The false and true names are extracted from
 * the \c std::numpunct facet of the input stream's locale (if the C++
 * standard library implementation supports locales), or from the \c
 * default_false_name and \c default_true_name functions (if there is
 * no locale support). The indeterminate name is extracted from the
 * appropriate \c indeterminate_name facet (if it is available in the
 * input stream's locale), or from the \c get_default_indeterminate_name
 * function (if the C++ standard library implementation does not
 * support locales, or the \c indeterminate_name facet is not
 * specified for this locale object). The input is then matched to
 * each of these names, and the tribool @p x is assigned the value
 * corresponding to the longest name that matched. If no name is
 * matched or all names are empty, the operation fails (and the fail
 * bit is set on the input stream @p in).
 *
 * \returns @p in
 */
template<typename CharT, typename Traits>
inline std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& in, tribool& x)
{
  if (in.flags() & std::ios_base::boolalpha) {
    typename std::basic_istream<CharT, Traits>::sentry cerberus(in);
    if (cerberus) {
      typedef std::basic_string<CharT> string_type;

#ifndef BOOST_NO_STD_LOCALE
      const std::numpunct<CharT>& numpunct_facet =
        BOOST_USE_FACET(std::numpunct<CharT>, in.getloc());

      string_type falsename = numpunct_facet.falsename();
      string_type truename = numpunct_facet.truename();

      string_type othername;
      if (BOOST_HAS_FACET(indeterminate_name<CharT>, in.getloc())) {
        othername =
          BOOST_USE_FACET(indeterminate_name<CharT>, in.getloc()).name();
      } else {
        othername = get_default_indeterminate_name<CharT>();
      }
#else
      string_type falsename = default_false_name<CharT>();
      string_type truename = default_true_name<CharT>();
      string_type othername = get_default_indeterminate_name<CharT>();
#endif

      typename string_type::size_type pos = 0;
      bool falsename_ok = true, truename_ok = true, othername_ok = true;

      // Modeled after the code from Library DR 17
      while ((falsename_ok && pos < falsename.size())
             || (truename_ok && pos < truename.size())
             || (othername_ok && pos < othername.size())) {
        typename Traits::int_type c = in.get();
        if (c == Traits::eof())
          return in;

        bool matched = false;
        if (falsename_ok && pos < falsename.size()) {
          if (Traits::eq(Traits::to_char_type(c), falsename[pos]))
            matched = true;
          else
            falsename_ok = false;
        }

        if (truename_ok && pos < truename.size()) {
          if (Traits::eq(Traits::to_char_type(c), truename[pos]))
            matched = true;
          else
            truename_ok = false;
        }

        if (othername_ok && pos < othername.size()) {
          if (Traits::eq(Traits::to_char_type(c), othername[pos]))
            matched = true;
          else
            othername_ok = false;
        }

        if (matched) { ++pos; }
        if (pos > falsename.size()) falsename_ok = false;
        if (pos > truename.size())  truename_ok = false;
        if (pos > othername.size()) othername_ok = false;
      }

      if (pos == 0)
        in.setstate(std::ios_base::failbit);
      else {
        if (falsename_ok)      x = false;
        else if (truename_ok)  x = true;
        else if (othername_ok) x = indeterminate;
        else in.setstate(std::ios_base::failbit);
      }
    }
  } else {
    long value;
    if (in >> value) {
      switch (value) {
      case 0: x = false; break;
      case 1: x = true; break;
      case 2: x = indeterminate; break;
      default: in.setstate(std::ios_base::failbit); break;
      }
    }
  }

  return in;
}

} } // end namespace boost::logic

#endif // BOOST_LOGIC_TRIBOOL_IO_HPP
