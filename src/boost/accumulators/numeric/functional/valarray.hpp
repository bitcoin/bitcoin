///////////////////////////////////////////////////////////////////////////////
/// \file valarray.hpp
///
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_NUMERIC_FUNCTIONAL_VALARRAY_HPP_EAN_12_12_2005
#define BOOST_NUMERIC_FUNCTIONAL_VALARRAY_HPP_EAN_12_12_2005

#ifdef BOOST_NUMERIC_FUNCTIONAL_HPP_INCLUDED
# error Include this file before boost/accumulators/numeric/functional.hpp
#endif

#include <valarray>
#include <functional>
#include <boost/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/typeof/std/valarray.hpp>
#include <boost/accumulators/numeric/functional_fwd.hpp>

namespace boost { namespace numeric
{
    namespace operators
    {
        namespace acc_detail
        {
            template<typename Fun>
            struct make_valarray
            {
                typedef std::valarray<typename Fun::result_type> type;
            };
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle valarray<Left> / Right where Right is a scalar and Right != Left.
        template<typename Left, typename Right>
        typename lazy_enable_if<
            mpl::and_<is_scalar<Right>, mpl::not_<is_same<Left, Right> > >
          , acc_detail::make_valarray<functional::divides<Left, Right> >
        >::type
        operator /(std::valarray<Left> const &left, Right const &right)
        {
            typedef typename functional::divides<Left, Right>::result_type value_type;
            std::valarray<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::divides(left[i], right);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle valarray<Left> * Right where Right is a scalar and Right != Left.
        template<typename Left, typename Right>
        typename lazy_enable_if<
            mpl::and_<is_scalar<Right>, mpl::not_<is_same<Left, Right> > >
          , acc_detail::make_valarray<functional::multiplies<Left, Right> >
        >::type
        operator *(std::valarray<Left> const &left, Right const &right)
        {
            typedef typename functional::multiplies<Left, Right>::result_type value_type;
            std::valarray<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::multiplies(left[i], right);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle valarray<Left> + valarray<Right> where Right != Left.
        template<typename Left, typename Right>
        typename lazy_disable_if<
            is_same<Left, Right>
          , acc_detail::make_valarray<functional::plus<Left, Right> >
        >::type
        operator +(std::valarray<Left> const &left, std::valarray<Right> const &right)
        {
            typedef typename functional::plus<Left, Right>::result_type value_type;
            std::valarray<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::plus(left[i], right[i]);
            }
            return result;
        }
    }

    namespace functional
    {
        struct std_valarray_tag;

        template<typename T>
        struct tag<std::valarray<T> >
        {
            typedef std_valarray_tag type;
        };

    #ifdef __GLIBCXX__
        template<typename T, typename U>
        struct tag<std::_Expr<T, U> >
        {
            typedef std_valarray_tag type;
        };
    #endif

        /// INTERNAL ONLY
        ///
        // This is necessary because the GCC stdlib uses expression templates, and
        // typeof(som-valarray-expression) is not an instance of std::valarray
    #define BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(Name, Op)                   \
        template<typename Left, typename Right>                                         \
        struct Name<Left, Right, std_valarray_tag, std_valarray_tag>                    \
        {                                                                               \
            typedef Left first_argument_type;                                           \
            typedef Right second_argument_type;                                         \
            typedef typename Left::value_type left_value_type;                          \
            typedef typename Right::value_type right_value_type;                        \
            typedef                                                                     \
                std::valarray<                                                          \
                    typename Name<left_value_type, right_value_type>::result_type       \
                >                                                                       \
            result_type;                                                                \
            result_type                                                                 \
            operator ()(Left &left, Right &right) const                                 \
            {                                                                           \
                return numeric::promote<std::valarray<left_value_type> >(left)          \
                    Op numeric::promote<std::valarray<right_value_type> >(right);       \
            }                                                                           \
        };                                                                              \
        template<typename Left, typename Right>                                         \
        struct Name<Left, Right, std_valarray_tag, void>                                \
        {                                                                               \
            typedef Left first_argument_type;                                           \
            typedef Right second_argument_type;                                         \
            typedef typename Left::value_type left_value_type;                          \
            typedef                                                                     \
                std::valarray<                                                          \
                    typename Name<left_value_type, Right>::result_type                  \
                >                                                                       \
            result_type;                                                                \
            result_type                                                                 \
            operator ()(Left &left, Right &right) const                                 \
            {                                                                           \
                return numeric::promote<std::valarray<left_value_type> >(left) Op right;\
            }                                                                           \
        };                                                                              \
        template<typename Left, typename Right>                                         \
        struct Name<Left, Right, void, std_valarray_tag>                                \
        {                                                                               \
            typedef Left first_argument_type;                                           \
            typedef Right second_argument_type;                                         \
            typedef typename Right::value_type right_value_type;                        \
            typedef                                                                     \
                std::valarray<                                                          \
                    typename Name<Left, right_value_type>::result_type                  \
                >                                                                       \
            result_type;                                                                \
            result_type                                                                 \
            operator ()(Left &left, Right &right) const                                 \
            {                                                                           \
                return left Op numeric::promote<std::valarray<right_value_type> >(right);\
            }                                                                           \
        };

        BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(plus, +)
        BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(minus, -)
        BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(multiplies, *)
        BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(divides, /)
        BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP(modulus, %)

    #undef BOOST_NUMERIC_FUNCTIONAL_DEFINE_VALARRAY_BIN_OP

        ///////////////////////////////////////////////////////////////////////////////
        // element-wise min of std::valarray
        template<typename Left, typename Right>
        struct min_assign<Left, Right, std_valarray_tag, std_valarray_tag>
        {
            typedef Left first_argument_type;
            typedef Right second_argument_type;
            typedef void result_type;

            void operator ()(Left &left, Right &right) const
            {
                BOOST_ASSERT(left.size() == right.size());
                for(std::size_t i = 0, size = left.size(); i != size; ++i)
                {
                    if(numeric::less(right[i], left[i]))
                    {
                        left[i] = right[i];
                    }
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // element-wise max of std::valarray
        template<typename Left, typename Right>
        struct max_assign<Left, Right, std_valarray_tag, std_valarray_tag>
        {
            typedef Left first_argument_type;
            typedef Right second_argument_type;
            typedef void result_type;

            void operator ()(Left &left, Right &right) const
            {
                BOOST_ASSERT(left.size() == right.size());
                for(std::size_t i = 0, size = left.size(); i != size; ++i)
                {
                    if(numeric::greater(right[i], left[i]))
                    {
                        left[i] = right[i];
                    }
                }
            }
        };

        // partial specialization of numeric::fdiv<> for std::valarray.
        template<typename Left, typename Right, typename RightTag>
        struct fdiv<Left, Right, std_valarray_tag, RightTag>
          : mpl::if_<
                are_integral<typename Left::value_type, Right>
              , divides<Left, double const>
              , divides<Left, Right>
            >::type
        {};

        // promote
        template<typename To, typename From>
        struct promote<To, From, std_valarray_tag, std_valarray_tag>
        {
            typedef From argument_type;
            typedef To result_type;

            To operator ()(From &arr) const
            {
                typename remove_const<To>::type res(arr.size());
                for(std::size_t i = 0, size = arr.size(); i != size; ++i)
                {
                    res[i] = numeric::promote<typename To::value_type>(arr[i]);
                }
                return res;
            }
        };

        template<typename ToFrom>
        struct promote<ToFrom, ToFrom, std_valarray_tag, std_valarray_tag>
        {
            typedef ToFrom argument_type;
            typedef ToFrom result_type;

            ToFrom &operator ()(ToFrom &tofrom) const
            {
                return tofrom;
            }
        };

        // for "promoting" a std::valarray<bool> to a bool, useful for
        // comparing 2 valarrays for equality:
        //   if(numeric::promote<bool>(a == b))
        template<typename From>
        struct promote<bool, From, void, std_valarray_tag>
        {
            typedef From argument_type;
            typedef bool result_type;

            bool operator ()(From &arr) const
            {
                BOOST_MPL_ASSERT((is_same<bool, typename From::value_type>));
                for(std::size_t i = 0, size = arr.size(); i != size; ++i)
                {
                    if(!arr[i])
                    {
                        return false;
                    }
                }
                return true;
            }
        };

        template<typename From>
        struct promote<bool const, From, void, std_valarray_tag>
          : promote<bool, From, void, std_valarray_tag>
        {};

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_min
        template<typename T>
        struct as_min<T, std_valarray_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(numeric::as_min(arr[0]), arr.size());
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_max
        template<typename T>
        struct as_max<T, std_valarray_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(numeric::as_max(arr[0]), arr.size());
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_zero
        template<typename T>
        struct as_zero<T, std_valarray_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(numeric::as_zero(arr[0]), arr.size());
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_one
        template<typename T>
        struct as_one<T, std_valarray_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(numeric::as_one(arr[0]), arr.size());
            }
        };

    } // namespace functional

}} // namespace boost::numeric

#endif

