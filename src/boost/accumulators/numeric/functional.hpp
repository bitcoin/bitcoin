///////////////////////////////////////////////////////////////////////////////
/// \file functional.hpp
///
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_NUMERIC_FUNCTIONAL_HPP_EAN_08_12_2005
#define BOOST_NUMERIC_FUNCTIONAL_HPP_EAN_08_12_2005

#include <limits>
#include <functional>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>
#include <boost/accumulators/numeric/functional_fwd.hpp>
#include <boost/accumulators/numeric/detail/function1.hpp>
#include <boost/accumulators/numeric/detail/function2.hpp>
#include <boost/accumulators/numeric/detail/pod_singleton.hpp>

#ifdef BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT
# include <boost/accumulators/numeric/functional/vector.hpp>
#endif

#ifdef BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
# include <boost/accumulators/numeric/functional/valarray.hpp>
#endif

#ifdef BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
# include <boost/accumulators/numeric/functional/complex.hpp>
#endif

/// INTERNAL ONLY
///
#define BOOST_NUMERIC_FUNCTIONAL_HPP_INCLUDED

#ifdef BOOST_NUMERIC_FUNCTIONAL_DOXYGEN_INVOKED
// Hack to make Doxygen show the inheritance relationships
/// INTERNAL ONLY
///
namespace std
{
    /// INTERNAL ONLY
    ///
    template<class Arg, class Ret> struct unary_function {};
    /// INTERNAL ONLY
    ///
    template<class Left, class Right, class Ret> struct binary_function {};
}
#endif

namespace boost { namespace numeric
{
    namespace functional
    {
        /// INTERNAL ONLY
        ///
        template<typename A0, typename A1>
        struct are_integral
          : mpl::and_<is_integral<A0>, is_integral<A1> >
        {};

        template<typename Left, typename Right>
        struct left_ref
        {
            typedef Left &type;
        };

        namespace detail
        {
            template<typename T>
            T &lvalue_of();
        }
    }

    // TODO: handle complex weight, valarray, MTL vectors

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP(Name, Op)                                      \
    namespace functional                                                                        \
    {                                                                                           \
        template<typename Arg>                                                                  \
        struct result_of_ ## Name                                                               \
        {                                                                                       \
            BOOST_TYPEOF_NESTED_TYPEDEF_TPL(                                                    \
                nested                                                                          \
              , Op boost::numeric::functional::detail::lvalue_of<Arg>()                         \
            )                                                                                   \
            typedef typename nested::type type;                                                 \
        };                                                                                      \
        template<typename Arg, typename EnableIf>                                               \
        struct Name ## _base                                                                    \
        {                                                                                       \
            typedef typename remove_const<Arg>::type argument_type;                             \
            typedef typename result_of_ ## Name<Arg>::type result_type;                         \
            typename result_of_ ## Name<Arg>::type operator ()(Arg &arg) const                  \
            {                                                                                   \
                return Op arg;                                                                  \
            }                                                                                   \
        };                                                                                      \
        template<typename Arg, typename ArgTag>                                                 \
        struct Name                                                                             \
          : Name ## _base<Arg, void>                                                            \
        {};                                                                                     \
    }                                                                                           \
    namespace op                                                                                \
    {                                                                                           \
        struct Name                                                                             \
          : boost::detail::function1<functional::Name<_, functional::tag<_> > >                 \
        {};                                                                                     \
    }                                                                                           \
    namespace                                                                                   \
    {                                                                                           \
        op::Name const &Name = boost::detail::pod_singleton<op::Name>::instance;                \
    }                                                                                           \
    /**/

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(Name, Op, RetType)                            \
    namespace functional                                                                        \
    {                                                                                           \
        template<typename Left, typename Right, typename EnableIf>                              \
        struct result_of_ ## Name                                                               \
        {                                                                                       \
            RetType(Left, Op, Right)                                                            \
        };                                                                                      \
        template<typename Left, typename Right, typename EnableIf>                              \
        struct Name ## _base                                                                    \
        {                                                                                       \
            typedef typename remove_const<Left>::type first_argument_type;                      \
            typedef typename remove_const<Right>::type second_argument_type;                    \
            typedef typename result_of_ ## Name<Left, Right>::type result_type;                 \
            typename result_of_ ## Name<Left, Right>::type                                      \
            operator ()(Left &left, Right &right) const                                         \
            {                                                                                   \
                return left Op right;                                                           \
            }                                                                                   \
        };                                                                                      \
        template<typename Left, typename Right, typename LeftTag, typename RightTag>            \
        struct Name                                                                             \
          : Name ## _base<Left, Right, void>                                                    \
        {};                                                                                     \
    }                                                                                           \
    namespace op                                                                                \
    {                                                                                           \
        struct Name                                                                             \
          : boost::detail::function2<                                                           \
                functional::Name<_1, _2, functional::tag<_1>, functional::tag<_2> >             \
            >                                                                                   \
        {};                                                                                     \
    }                                                                                           \
    namespace                                                                                   \
    {                                                                                           \
        op::Name const &Name = boost::detail::pod_singleton<op::Name>::instance;                \
    }                                                                                           \
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(Name)                                                      \
    /**/

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_DEDUCED(Left, Op, Right)                                       \
    BOOST_TYPEOF_NESTED_TYPEDEF_TPL(                                                            \
        nested                                                                                  \
      , boost::numeric::functional::detail::lvalue_of<Left>() Op                                \
        boost::numeric::functional::detail::lvalue_of<Right>()                                  \
    )                                                                                           \
    typedef typename nested::type type;                                                         \
    /**/

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_LEFT(Left, Op, Right)                                          \
    typedef Left &type;                                                                         \
    /**/

    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(plus, +, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(minus, -, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(multiplies, *, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(divides, /, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(modulus, %, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(greater, >, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(greater_equal, >=, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(less, <, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(less_equal, <=, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(equal_to, ==, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(not_equal_to, !=, BOOST_NUMERIC_FUNCTIONAL_DEDUCED)

    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(assign, =, BOOST_NUMERIC_FUNCTIONAL_LEFT)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(plus_assign, +=, BOOST_NUMERIC_FUNCTIONAL_LEFT)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(minus_assign, -=, BOOST_NUMERIC_FUNCTIONAL_LEFT)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(multiplies_assign, *=, BOOST_NUMERIC_FUNCTIONAL_LEFT)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(divides_assign, /=, BOOST_NUMERIC_FUNCTIONAL_LEFT)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP(modulus_assign, %=, BOOST_NUMERIC_FUNCTIONAL_LEFT)

    BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP(unary_plus, +)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP(unary_minus, -)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP(complement, ~)
    BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP(logical_not, !)

#undef BOOST_NUMERIC_FUNCTIONAL_LEFT
#undef BOOST_NUMERIC_FUNCTIONAL_DEDUCED
#undef BOOST_NUMERIC_FUNCTIONAL_DEFINE_UNARY_OP
#undef BOOST_NUMERIC_FUNCTIONAL_DEFINE_BINARY_OP

    namespace functional
    {
        template<typename Left, typename Right, typename EnableIf>
        struct min_assign_base
        {
            typedef Left first_argument_type;
            typedef Right second_argument_type;
            typedef void result_type;

            void operator ()(Left &left, Right &right) const
            {
                if(numeric::less(right, left))
                {
                    left = right;
                }
            }
        };

        template<typename Left, typename Right, typename EnableIf>
        struct max_assign_base
        {
            typedef Left first_argument_type;
            typedef Right second_argument_type;
            typedef void result_type;

            void operator ()(Left &left, Right &right) const
            {
                if(numeric::greater(right, left))
                {
                    left = right;
                }
            }
        };

        template<typename Left, typename Right, typename EnableIf>
        struct fdiv_base
          : functional::divides<Left, Right>
        {};

        // partial specialization that promotes the arguments to double for
        // integral division.
        template<typename Left, typename Right>
        struct fdiv_base<Left, Right, typename enable_if<are_integral<Left, Right> >::type>
          : functional::divides<double const, double const>
        {};

        template<typename To, typename From, typename EnableIf>
        struct promote_base
        {
            typedef From argument_type;
            typedef To result_type;

            To operator ()(From &from) const
            {
                return from;
            }
        };

        template<typename ToFrom>
        struct promote_base<ToFrom, ToFrom, void>
        {
            typedef ToFrom argument_type;
            typedef ToFrom result_type;

            ToFrom &operator ()(ToFrom &tofrom)
            {
                return tofrom;
            }
        };

        template<typename Arg, typename EnableIf>
        struct as_min_base
        {
            BOOST_STATIC_ASSERT(std::numeric_limits<typename remove_const<Arg>::type>::is_specialized);

            typedef Arg argument_type;
            typedef typename remove_const<Arg>::type result_type;

            typename remove_const<Arg>::type operator ()(Arg &) const
            {
                return (std::numeric_limits<typename remove_const<Arg>::type>::min)();
            }
        };

        template<typename Arg>
        struct as_min_base<Arg, typename enable_if<is_floating_point<Arg> >::type>
        {
            BOOST_STATIC_ASSERT(std::numeric_limits<typename remove_const<Arg>::type>::is_specialized);

            typedef Arg argument_type;
            typedef typename remove_const<Arg>::type result_type;

            typename remove_const<Arg>::type operator ()(Arg &) const
            {
                return -(std::numeric_limits<typename remove_const<Arg>::type>::max)();
            }
        };

        template<typename Arg, typename EnableIf>
        struct as_max_base
        {
            BOOST_STATIC_ASSERT(std::numeric_limits<typename remove_const<Arg>::type>::is_specialized);

            typedef Arg argument_type;
            typedef typename remove_const<Arg>::type result_type;

            typename remove_const<Arg>::type operator ()(Arg &) const
            {
                return (std::numeric_limits<typename remove_const<Arg>::type>::max)();
            }
        };

        template<typename Arg, typename EnableIf>
        struct as_zero_base
        {
            typedef Arg argument_type;
            typedef typename remove_const<Arg>::type result_type;

            typename remove_const<Arg>::type operator ()(Arg &) const
            {
                return numeric::zero<typename remove_const<Arg>::type>::value;
            }
        };

        template<typename Arg, typename EnableIf>
        struct as_one_base
        {
            typedef Arg argument_type;
            typedef typename remove_const<Arg>::type result_type;

            typename remove_const<Arg>::type operator ()(Arg &) const
            {
                return numeric::one<typename remove_const<Arg>::type>::value;
            }
        };

        template<typename To, typename From, typename ToTag, typename FromTag>
        struct promote
          : promote_base<To, From, void>
        {};

        template<typename Left, typename Right, typename LeftTag, typename RightTag>
        struct min_assign
          : min_assign_base<Left, Right, void>
        {};

        template<typename Left, typename Right, typename LeftTag, typename RightTag>
        struct max_assign
          : max_assign_base<Left, Right, void>
        {};

        template<typename Left, typename Right, typename LeftTag, typename RightTag>
        struct fdiv
          : fdiv_base<Left, Right, void>
        {};

        /// INTERNAL ONLY 
        /// For back-compat only. Use fdiv.
        template<typename Left, typename Right, typename LeftTag, typename RightTag>
        struct average
          : fdiv<Left, Right, LeftTag, RightTag>
        {};

        template<typename Arg, typename Tag>
        struct as_min
          : as_min_base<Arg, void>
        {};

        template<typename Arg, typename Tag>
        struct as_max
          : as_max_base<Arg, void>
        {};

        template<typename Arg, typename Tag>
        struct as_zero
          : as_zero_base<Arg, void>
        {};

        template<typename Arg, typename Tag>
        struct as_one
          : as_one_base<Arg, void>
        {};
    }

    namespace op
    {
        template<typename To>
        struct promote
          : boost::detail::function1<functional::promote<To, _, typename functional::tag<To>::type, functional::tag<_> > >
        {};

        struct min_assign
          : boost::detail::function2<functional::min_assign<_1, _2, functional::tag<_1>, functional::tag<_2> > >
        {};

        struct max_assign
          : boost::detail::function2<functional::max_assign<_1, _2, functional::tag<_1>, functional::tag<_2> > >
        {};

        struct fdiv
          : boost::detail::function2<functional::fdiv<_1, _2, functional::tag<_1>, functional::tag<_2> > >
        {};

        /// INTERNAL ONLY
        struct average
          : boost::detail::function2<functional::fdiv<_1, _2, functional::tag<_1>, functional::tag<_2> > >
        {};

        struct as_min
          : boost::detail::function1<functional::as_min<_, functional::tag<_> > >
        {};

        struct as_max
          : boost::detail::function1<functional::as_max<_, functional::tag<_> > >
        {};

        struct as_zero
          : boost::detail::function1<functional::as_zero<_, functional::tag<_> > >
        {};

        struct as_one
          : boost::detail::function1<functional::as_one<_, functional::tag<_> > >
        {};
    }

    namespace
    {
        op::min_assign const &min_assign = boost::detail::pod_singleton<op::min_assign>::instance;
        op::max_assign const &max_assign = boost::detail::pod_singleton<op::max_assign>::instance;
        op::fdiv const &fdiv = boost::detail::pod_singleton<op::fdiv>::instance;
        op::fdiv const &average = boost::detail::pod_singleton<op::fdiv>::instance; ///< INTERNAL ONLY
        op::as_min const &as_min = boost::detail::pod_singleton<op::as_min>::instance;
        op::as_max const &as_max = boost::detail::pod_singleton<op::as_max>::instance;
        op::as_zero const &as_zero = boost::detail::pod_singleton<op::as_zero>::instance;
        op::as_one const &as_one = boost::detail::pod_singleton<op::as_one>::instance;

        BOOST_ACCUMULATORS_IGNORE_GLOBAL(min_assign)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(max_assign)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(fdiv)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(average)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(as_min)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(as_max)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(as_zero)
        BOOST_ACCUMULATORS_IGNORE_GLOBAL(as_one)
    }

    ///////////////////////////////////////////////////////////////////////////////
    // promote
    template<typename To, typename From>
    typename lazy_disable_if<is_const<From>, mpl::if_<is_same<To, From>, To &, To> >::type
    promote(From &from)
    {
        return functional::promote<To, From>()(from);
    }

    template<typename To, typename From>
    typename mpl::if_<is_same<To const, From const>, To const &, To const>::type
    promote(From const &from)
    {
        return functional::promote<To const, From const>()(from);
    }

    template<typename T>
    struct default_
    {
        typedef default_ type;
        typedef T value_type;
        static T const value;

        operator T const & () const
        {
            return default_::value;
        }
    };

    template<typename T>
    T const default_<T>::value = T();

    template<typename T>
    struct one
    {
        typedef one type;
        typedef T value_type;
        static T const value;

        operator T const & () const
        {
            return one::value;
        }
    };

    template<typename T>
    T const one<T>::value = T(1);

    template<typename T>
    struct zero
    {
        typedef zero type;
        typedef T value_type;
        static T const value;

        operator T const & () const
        {
            return zero::value;
        }
    };

    template<typename T>
    T const zero<T>::value = T();

    template<typename T>
    struct one_or_default
      : mpl::if_<is_empty<T>, default_<T>, one<T> >::type
    {};

    template<typename T>
    struct zero_or_default
      : mpl::if_<is_empty<T>, default_<T>, zero<T> >::type
    {};

}} // namespace boost::numeric

#endif
