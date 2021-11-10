// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef KEYWORDS_DWA2002323_HPP
# define KEYWORDS_DWA2002323_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/args_fwd.hpp>
# include <boost/config.hpp>
# include <boost/python/detail/preprocessor.hpp>
# include <boost/python/detail/type_list.hpp>
# include <boost/python/detail/type_traits.hpp>

# include <boost/preprocessor/enum_params.hpp>
# include <boost/preprocessor/repeat.hpp>
# include <boost/preprocessor/facilities/intercept.hpp>
# include <boost/preprocessor/iteration/local.hpp>

# include <boost/python/detail/mpl_lambda.hpp>
# include <boost/python/object_core.hpp>

# include <boost/mpl/bool.hpp>

# include <cstddef>
# include <algorithm>

namespace boost { namespace python {

typedef detail::keywords<1> arg;
typedef arg arg_; // gcc 2.96 workaround

namespace detail
{
  template <std::size_t nkeywords>
  struct keywords_base
  {
      BOOST_STATIC_CONSTANT(std::size_t, size = nkeywords);
      
      keyword_range range() const
      {
          return keyword_range(elements, elements + nkeywords);
      }

      keyword elements[nkeywords];

      keywords<nkeywords+1>
      operator,(python::arg const &k) const;

      keywords<nkeywords + 1>
      operator,(char const *name) const;
  };
  
  template <std::size_t nkeywords>
  struct keywords : keywords_base<nkeywords>
  {
  };

  template <>
  struct keywords<1> : keywords_base<1>
  {
      explicit keywords(char const *name)
      {
          elements[0].name = name;
      }
    
      template <class T>
      python::arg& operator=(T const& value)
      {
          object z(value);
          elements[0].default_value = handle<>(python::borrowed(object(value).ptr()));
          return *this;
      }
    
      operator detail::keyword const&() const
      {
          return elements[0];
      }
  };

  template <std::size_t nkeywords>
  inline
  keywords<nkeywords+1>
  keywords_base<nkeywords>::operator,(python::arg const &k) const
  {
      keywords<nkeywords> const& l = *static_cast<keywords<nkeywords> const*>(this);
      python::detail::keywords<nkeywords+1> res;
      std::copy(l.elements, l.elements+nkeywords, res.elements);
      res.elements[nkeywords] = k.elements[0];
      return res;
  }

  template <std::size_t nkeywords>
  inline
  keywords<nkeywords + 1>
  keywords_base<nkeywords>::operator,(char const *name) const
  {
      return this->operator,(python::arg(name));
  }

  template<typename T>
  struct is_keywords
  {
      BOOST_STATIC_CONSTANT(bool, value = false); 
  };

  template<std::size_t nkeywords>
  struct is_keywords<keywords<nkeywords> >
  {
      BOOST_STATIC_CONSTANT(bool, value = true);
  };
  template <class T>
  struct is_reference_to_keywords
  {
      BOOST_STATIC_CONSTANT(bool, is_ref = detail::is_reference<T>::value);
      typedef typename detail::remove_reference<T>::type deref;
      typedef typename detail::remove_cv<deref>::type key_t;
      BOOST_STATIC_CONSTANT(bool, is_key = is_keywords<key_t>::value);
      BOOST_STATIC_CONSTANT(bool, value = (is_ref & is_key));
      
      typedef mpl::bool_<value> type;
      BOOST_PYTHON_MPL_LAMBDA_SUPPORT(1,is_reference_to_keywords,(T))
  };
}

inline detail::keywords<1> args(char const* name)
{ 
    return detail::keywords<1>(name);
}

#  define BOOST_PYTHON_ASSIGN_NAME(z, n, _) result.elements[n].name = name##n;
#  define BOOST_PP_LOCAL_MACRO(n)                                               \
inline detail::keywords<n> args(BOOST_PP_ENUM_PARAMS_Z(1, n, char const* name)) \
{                                                                               \
    detail::keywords<n> result;                                                 \
    BOOST_PP_REPEAT_1(n, BOOST_PYTHON_ASSIGN_NAME, _)                           \
    return result;                                                              \
}
#  define BOOST_PP_LOCAL_LIMITS (2, BOOST_PYTHON_MAX_ARITY)
#  include BOOST_PP_LOCAL_ITERATE()

}} // namespace boost::python


# endif // KEYWORDS_DWA2002323_HPP
