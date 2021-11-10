/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_MODIFY_KEY_ADAPTOR_HPP
#define BOOST_MULTI_INDEX_DETAIL_MODIFY_KEY_ADAPTOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* Functional adaptor to resolve modify_key as a call to modify.
 * Preferred over compose_f_gx and stuff cause it eliminates problems
 * with references to references, dealing with function pointers, etc.
 */

template<typename Fun,typename Value,typename KeyFromValue>
struct modify_key_adaptor
{

  modify_key_adaptor(Fun f_,KeyFromValue kfv_):f(f_),kfv(kfv_){}

  void operator()(Value& x)
  {
    f(kfv(x));
  }

private:
  Fun          f;
  KeyFromValue kfv;
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
