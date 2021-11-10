// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef UNWRAP_WRAPPER_DWA2004723_HPP
# define UNWRAP_WRAPPER_DWA2004723_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/is_wrapper.hpp>
# include <boost/mpl/eval_if.hpp>
# include <boost/mpl/identity.hpp>

namespace boost { namespace python { namespace detail { 

template <class T>
struct unwrap_wrapper_helper
{
    typedef typename T::_wrapper_wrapped_type_ type;
};

template <class T>
struct unwrap_wrapper_
  : mpl::eval_if<is_wrapper<T>,unwrap_wrapper_helper<T>,mpl::identity<T> >
{};

template <class T>
typename unwrap_wrapper_<T>::type*
unwrap_wrapper(T*)
{
    return 0;
}

}}} // namespace boost::python::detail

#endif // UNWRAP_WRAPPER_DWA2004723_HPP
