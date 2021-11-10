#ifndef BOOST_QVM_ENABLE_IF_HPP_INCLUDED
#define BOOST_QVM_ENABLE_IF_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

namespace boost { namespace qvm {

  template<typename T, typename R=void>
  struct enable_if_has_type
  {
    typedef R type;
  };

  template <bool B, class T = void>
  struct enable_if_c {
    typedef T type;
  };

  template <class T>
  struct enable_if_c<false, T> {};

  template <class Cond, class T = void>
  struct enable_if : public enable_if_c<Cond::value, T> {};

  template <bool B, class T>
  struct lazy_enable_if_c {
    typedef typename T::type type;
  };

  template <class T>
  struct lazy_enable_if_c<false, T> {};

  template <class Cond, class T>
  struct lazy_enable_if : public lazy_enable_if_c<Cond::value, T> {};


  template <bool B, class T = void>
  struct disable_if_c {
    typedef T type;
  };

  template <class T>
  struct disable_if_c<true, T> {};

  template <class Cond, class T = void>
  struct disable_if : public disable_if_c<Cond::value, T> {};

  template <bool B, class T>
  struct lazy_disable_if_c {
    typedef typename T::type type;
  };

  template <class T>
  struct lazy_disable_if_c<true, T> {};

  template <class Cond, class T>
  struct lazy_disable_if : public lazy_disable_if_c<Cond::value, T> {};

////////////////////////////////////////////////

  // The types below are a copy of the original types above, to workaround MSVC-12 bugs.

  template<typename T, typename R=void>
  struct enable_if_has_type2
  {
    typedef R type;
  };

  template <bool B, class T = void>
  struct enable_if_c2 {
    typedef T type;
  };

  template <class T>
  struct enable_if_c2<false, T> {};

  template <class Cond, class T = void>
  struct enable_if2 : public enable_if_c2<Cond::value, T> {};

  template <bool B, class T>
  struct lazy_enable_if_c2 {
    typedef typename T::type type;
  };

  template <class T>
  struct lazy_enable_if_c2<false, T> {};

  template <class Cond, class T>
  struct lazy_enable_if2 : public lazy_enable_if_c2<Cond::value, T> {};


  template <bool B, class T = void>
  struct disable_if_c2 {
    typedef T type;
  };

  template <class T>
  struct disable_if_c2<true, T> {};

  template <class Cond, class T = void>
  struct disable_if2 : public disable_if_c2<Cond::value, T> {};

  template <bool B, class T>
  struct lazy_disable_if_c2 {
    typedef typename T::type type;
  };

  template <class T>
  struct lazy_disable_if_c2<true, T> {};

  template <class Cond, class T>
  struct lazy_disable_if2 : public lazy_disable_if_c2<Cond::value, T> {};

} }

#endif
