///////////////////////////////////////////////////////////////////////////////
/// \file vector.hpp
///
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_NUMERIC_FUNCTIONAL_VECTOR_HPP_EAN_12_12_2005
#define BOOST_NUMERIC_FUNCTIONAL_VECTOR_HPP_EAN_12_12_2005

#ifdef BOOST_NUMERIC_FUNCTIONAL_HPP_INCLUDED
# error Include this file before boost/accumulators/numeric/functional.hpp
#endif

#include <vector>
#include <functional>
#include <boost/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/typeof/std/vector.hpp>
#include <boost/accumulators/numeric/functional_fwd.hpp>

namespace boost { namespace numeric
{
    namespace operators
    {
        namespace acc_detail
        {
            template<typename Fun>
            struct make_vector
            {
                typedef std::vector<typename Fun::result_type> type;
            };
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> / Right where Right is a scalar.
        template<typename Left, typename Right>
        typename lazy_enable_if<
            is_scalar<Right>
          , acc_detail::make_vector<functional::divides<Left, Right> >
        >::type
        operator /(std::vector<Left> const &left, Right const &right)
        {
            typedef typename functional::divides<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::divides(left[i], right);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> / vector<Right>.
        template<typename Left, typename Right>
        std::vector<typename functional::divides<Left, Right>::result_type>
        operator /(std::vector<Left> const &left, std::vector<Right> const &right)
        {
            typedef typename functional::divides<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::divides(left[i], right[i]);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> * Right where Right is a scalar.
        template<typename Left, typename Right>
        typename lazy_enable_if<
            is_scalar<Right>
          , acc_detail::make_vector<functional::multiplies<Left, Right> >
        >::type
        operator *(std::vector<Left> const &left, Right const &right)
        {
            typedef typename functional::multiplies<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::multiplies(left[i], right);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle Left * vector<Right> where Left is a scalar.
        template<typename Left, typename Right>
        typename lazy_enable_if<
            is_scalar<Left>
          , acc_detail::make_vector<functional::multiplies<Left, Right> >
        >::type
        operator *(Left const &left, std::vector<Right> const &right)
        {
            typedef typename functional::multiplies<Left, Right>::result_type value_type;
            std::vector<value_type> result(right.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::multiplies(left, right[i]);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> * vector<Right>
        template<typename Left, typename Right>
        std::vector<typename functional::multiplies<Left, Right>::result_type>
        operator *(std::vector<Left> const &left, std::vector<Right> const &right)
        {
            typedef typename functional::multiplies<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::multiplies(left[i], right[i]);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> + vector<Right>
        template<typename Left, typename Right>
        std::vector<typename functional::plus<Left, Right>::result_type>
        operator +(std::vector<Left> const &left, std::vector<Right> const &right)
        {
            typedef typename functional::plus<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::plus(left[i], right[i]);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> - vector<Right>
        template<typename Left, typename Right>
        std::vector<typename functional::minus<Left, Right>::result_type>
        operator -(std::vector<Left> const &left, std::vector<Right> const &right)
        {
            typedef typename functional::minus<Left, Right>::result_type value_type;
            std::vector<value_type> result(left.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::minus(left[i], right[i]);
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle vector<Left> += vector<Left>
        template<typename Left>
        std::vector<Left> &
        operator +=(std::vector<Left> &left, std::vector<Left> const &right)
        {
            BOOST_ASSERT(left.size() == right.size());
            for(std::size_t i = 0, size = left.size(); i != size; ++i)
            {
                numeric::plus_assign(left[i], right[i]);
            }
            return left;
        }

        ///////////////////////////////////////////////////////////////////////////////
        // Handle -vector<Arg>
        template<typename Arg>
        std::vector<typename functional::unary_minus<Arg>::result_type>
        operator -(std::vector<Arg> const &arg)
        {
            typedef typename functional::unary_minus<Arg>::result_type value_type;
            std::vector<value_type> result(arg.size());
            for(std::size_t i = 0, size = result.size(); i != size; ++i)
            {
                result[i] = numeric::unary_minus(arg[i]);
            }
            return result;
        }
    }

    namespace functional
    {
        struct std_vector_tag;

        template<typename T, typename Al>
        struct tag<std::vector<T, Al> >
        {
            typedef std_vector_tag type;
        };

        ///////////////////////////////////////////////////////////////////////////////
        // element-wise min of std::vector
        template<typename Left, typename Right>
        struct min_assign<Left, Right, std_vector_tag, std_vector_tag>
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
        // element-wise max of std::vector
        template<typename Left, typename Right>
        struct max_assign<Left, Right, std_vector_tag, std_vector_tag>
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

        // partial specialization for std::vector.
        template<typename Left, typename Right>
        struct fdiv<Left, Right, std_vector_tag, void>
          : mpl::if_<
                are_integral<typename Left::value_type, Right>
              , divides<Left, double const>
              , divides<Left, Right>
            >::type
        {};

        // promote
        template<typename To, typename From>
        struct promote<To, From, std_vector_tag, std_vector_tag>
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
        struct promote<ToFrom, ToFrom, std_vector_tag, std_vector_tag>
        {
            typedef ToFrom argument_type;
            typedef ToFrom result_type;

            ToFrom &operator ()(ToFrom &tofrom) const
            {
                return tofrom;
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_min
        template<typename T>
        struct as_min<T, std_vector_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(arr.size(), numeric::as_min(arr[0]));
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_max
        template<typename T>
        struct as_max<T, std_vector_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(arr.size(), numeric::as_max(arr[0]));
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_zero
        template<typename T>
        struct as_zero<T, std_vector_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(arr.size(), numeric::as_zero(arr[0]));
            }
        };

        ///////////////////////////////////////////////////////////////////////////////
        // functional::as_one
        template<typename T>
        struct as_one<T, std_vector_tag>
        {
            typedef T argument_type;
            typedef typename remove_const<T>::type result_type;

            typename remove_const<T>::type operator ()(T &arr) const
            {
                return 0 == arr.size()
                  ? T()
                  : T(arr.size(), numeric::as_one(arr[0]));
            }
        };

    } // namespace functional

}} // namespace boost::numeric

#endif

