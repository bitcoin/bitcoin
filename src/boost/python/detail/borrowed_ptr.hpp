#ifndef BORROWED_PTR_DWA20020601_HPP
# define BORROWED_PTR_DWA20020601_HPP
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# include <boost/config.hpp>
# include <boost/type.hpp>
# include <boost/mpl/if.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/python/tag.hpp>

namespace boost { namespace python { namespace detail {

template<class T> class borrowed
{ 
    typedef T type;
};

template<typename T>
struct is_borrowed_ptr
{
    BOOST_STATIC_CONSTANT(bool, value = false); 
};

#  if !defined(__MWERKS__) || __MWERKS__ > 0x3000
template<typename T>
struct is_borrowed_ptr<borrowed<T>*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> const*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> volatile*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<typename T>
struct is_borrowed_ptr<borrowed<T> const volatile*>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};
#  else
template<typename T>
struct is_borrowed
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};
template<typename T>
struct is_borrowed<borrowed<T> >
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};
template<typename T>
struct is_borrowed_ptr<T*>
    : is_borrowed<typename remove_cv<T>::type>
{
};
#  endif 


}

template <class T>
inline T* get_managed_object(detail::borrowed<T> const volatile* p, tag_t)
{
    return (T*)p;
}

}} // namespace boost::python::detail

#endif // #ifndef BORROWED_PTR_DWA20020601_HPP
