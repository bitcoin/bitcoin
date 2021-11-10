// tuple_io.hpp --------------------------------------------------------------

// Copyright (C) 2001 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//               2001 Gary Powell (gary.powell@sierra.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

// ----------------------------------------------------------------------------

#ifndef BOOST_TUPLE_IO_HPP
#define BOOST_TUPLE_IO_HPP

#include <istream>
#include <ostream>

#include <sstream>

#include <boost/tuple/tuple.hpp>

// This is ugly: one should be using twoargument isspace since whitspace can
// be locale dependent, in theory at least.
// not all libraries implement have the two-arg version, so we need to
// use the one-arg one, which one should get with <cctype> but there seem
// to be exceptions to this.

#if !defined (BOOST_NO_STD_LOCALE)

#include <locale> // for two-arg isspace

#else

#include <cctype> // for one-arg (old) isspace
#include <ctype.h> // Metrowerks does not find one-arg isspace from cctype

#endif

namespace boost {
namespace tuples {

namespace detail {

class format_info {
public:

   enum manipulator_type { open, close, delimiter };
   BOOST_STATIC_CONSTANT(int, number_of_manipulators = delimiter + 1);
private:

   static int get_stream_index (int m)
   {
     static const int stream_index[number_of_manipulators]
        = { std::ios::xalloc(), std::ios::xalloc(), std::ios::xalloc() };

     return stream_index[m];
   }

   format_info(const format_info&);
   format_info();


public:

   template<class CharType, class CharTrait>
   static CharType get_manipulator(std::basic_ios<CharType, CharTrait>& i,
                                   manipulator_type m) {
     // The manipulators are stored as long.
     // A valid instanitation of basic_stream allows CharType to be any POD,
     // hence, the static_cast may fail (it fails if long is not convertible
     // to CharType
     CharType c = static_cast<CharType>(i.iword(get_stream_index(m)) );
     // parentheses and space are the default manipulators
     if (!c) {
       switch(m) {
         case detail::format_info::open :  c = i.widen('('); break;
         case detail::format_info::close : c = i.widen(')'); break;
         case detail::format_info::delimiter : c = i.widen(' '); break;
       }
     }
     return c;
   }


   template<class CharType, class CharTrait>
   static void set_manipulator(std::basic_ios<CharType, CharTrait>& i,
                               manipulator_type m, CharType c) {
     // The manipulators are stored as long.
     // A valid instanitation of basic_stream allows CharType to be any POD,
     // hence, the static_cast may fail (it fails if CharType is not
     // convertible long.
      i.iword(get_stream_index(m)) = static_cast<long>(c);
   }
};

} // end of namespace detail

template<class CharType>
class tuple_manipulator {
  const detail::format_info::manipulator_type mt;
  CharType f_c;
public:
  explicit tuple_manipulator(detail::format_info::manipulator_type m,
                             CharType c = CharType())
     : mt(m), f_c(c) {}

   template<class CharTrait>
  void set(std::basic_ios<CharType, CharTrait> &io) const {
     detail::format_info::set_manipulator(io, mt, f_c);
  }
};


template<class CharType, class CharTrait>
inline std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& o, const tuple_manipulator<CharType>& m) {
  m.set(o);
  return o;
}

template<class CharType, class CharTrait>
inline std::basic_istream<CharType, CharTrait>&
operator>>(std::basic_istream<CharType, CharTrait>& i, const tuple_manipulator<CharType>& m) {
  m.set(i);
  return i;
}


template<class CharType>
inline tuple_manipulator<CharType> set_open(const CharType c) {
   return tuple_manipulator<CharType>(detail::format_info::open, c);
}

template<class CharType>
inline tuple_manipulator<CharType> set_close(const CharType c) {
   return tuple_manipulator<CharType>(detail::format_info::close, c);
}

template<class CharType>
inline tuple_manipulator<CharType> set_delimiter(const CharType c) {
   return tuple_manipulator<CharType>(detail::format_info::delimiter, c);
}





// -------------------------------------------------------------
// printing tuples to ostream in format (a b c)
// parentheses and space are defaults, but can be overriden with manipulators
// set_open, set_close and set_delimiter

namespace detail {

// Note: The order of the print functions is critical
// to let a conforming compiler  find and select the correct one.


template<class CharType, class CharTrait, class T1>
inline std::basic_ostream<CharType, CharTrait>&
print(std::basic_ostream<CharType, CharTrait>& o, const cons<T1, null_type>& t) {
  return o << t.head;
}


template<class CharType, class CharTrait>
inline std::basic_ostream<CharType, CharTrait>&
print(std::basic_ostream<CharType, CharTrait>& o, const null_type&) {
  return o;
}

template<class CharType, class CharTrait, class T1, class T2>
inline std::basic_ostream<CharType, CharTrait>&
print(std::basic_ostream<CharType, CharTrait>& o, const cons<T1, T2>& t) {

  const CharType d = format_info::get_manipulator(o, format_info::delimiter);

  o << t.head;

  o << d;

  return print(o, t.tail);
}

template<class CharT, class Traits, class T>
inline bool handle_width(std::basic_ostream<CharT, Traits>& o, const T& t) {
    std::streamsize width = o.width();
    if(width == 0) return false;

    std::basic_ostringstream<CharT, Traits> ss;

    ss.copyfmt(o);
    ss.tie(0);
    ss.width(0);

    ss << t;
    o << ss.str();

    return true;
}


} // namespace detail


template<class CharType, class CharTrait>
inline std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& o,
           const null_type& t) {
  if (!o.good() ) return o;
  if (detail::handle_width(o, t)) return o;

  const CharType l =
    detail::format_info::get_manipulator(o, detail::format_info::open);
  const CharType r =
    detail::format_info::get_manipulator(o, detail::format_info::close);

  o << l;
  o << r;

  return o;
}

template<class CharType, class CharTrait, class T1, class T2>
inline std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& o,
           const cons<T1, T2>& t) {
  if (!o.good() ) return o;
  if (detail::handle_width(o, t)) return o;

  const CharType l =
    detail::format_info::get_manipulator(o, detail::format_info::open);
  const CharType r =
    detail::format_info::get_manipulator(o, detail::format_info::close);

  o << l;

  detail::print(o, t);

  o << r;

  return o;
}


// -------------------------------------------------------------
// input stream operators

namespace detail {


template<class CharType, class CharTrait>
inline std::basic_istream<CharType, CharTrait>&
extract_and_check_delimiter(
  std::basic_istream<CharType, CharTrait> &is, format_info::manipulator_type del)
{
  const CharType d = format_info::get_manipulator(is, del);

#if defined (BOOST_NO_STD_LOCALE)
  const bool is_delimiter = !isspace(d);
#elif defined ( BOOST_BORLANDC )
  const bool is_delimiter = !std::use_facet< std::ctype< CharType > >
    (is.getloc() ).is( std::ctype_base::space, d);
#else
  const bool is_delimiter = (!std::isspace(d, is.getloc()) );
#endif

  CharType c;
  if (is_delimiter) {
    is >> c;
    if (is.good() && c!=d) {
      is.setstate(std::ios::failbit);
    }
  } else {
    is >> std::ws;
  }
  return is;
}


template<class CharType, class CharTrait, class T1>
inline  std::basic_istream<CharType, CharTrait> &
read (std::basic_istream<CharType, CharTrait> &is, cons<T1, null_type>& t1) {

  if (!is.good()) return is;

  return is >> t1.head;
}

template<class CharType, class CharTrait, class T1, class T2>
inline std::basic_istream<CharType, CharTrait>&
read(std::basic_istream<CharType, CharTrait> &is, cons<T1, T2>& t1) {

  if (!is.good()) return is;

  is >> t1.head;


  extract_and_check_delimiter(is, format_info::delimiter);

  return read(is, t1.tail);
}

} // end namespace detail


template<class CharType, class CharTrait>
inline std::basic_istream<CharType, CharTrait>&
operator>>(std::basic_istream<CharType, CharTrait> &is, null_type&) {

  if (!is.good() ) return is;

  detail::extract_and_check_delimiter(is, detail::format_info::open);
  detail::extract_and_check_delimiter(is, detail::format_info::close);

  return is;
}

template<class CharType, class CharTrait, class T1, class T2>
inline std::basic_istream<CharType, CharTrait>&
operator>>(std::basic_istream<CharType, CharTrait>& is, cons<T1, T2>& t1) {

  if (!is.good() ) return is;

  detail::extract_and_check_delimiter(is, detail::format_info::open);

  detail::read(is, t1);

  detail::extract_and_check_delimiter(is, detail::format_info::close);

  return is;
}


} // end of namespace tuples
} // end of namespace boost

#endif // BOOST_TUPLE_IO_HPP
