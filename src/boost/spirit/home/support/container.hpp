/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONTAINER_FEBRUARY_06_2007_1001AM)
#define BOOST_SPIRIT_CONTAINER_FEBRUARY_06_2007_1001AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/attributes_fwd.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/range/range_fwd.hpp>
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  This file contains some container utils for stl containers. The
    //  utilities provided also accept spirit's unused_type; all no-ops.
    //  Compiler optimization will easily strip these away.
    ///////////////////////////////////////////////////////////////////////////

    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(size_type)
        BOOST_MPL_HAS_XXX_TRAIT_DEF(reference)
    }

    template <typename T, typename Enable/* = void*/>
    struct is_container
      : mpl::bool_<
            detail::has_value_type<T>::value &&
            detail::has_iterator<T>::value &&
            detail::has_size_type<T>::value &&
            detail::has_reference<T>::value>
    {};

    template <typename T>
    struct is_container<T&>
      : is_container<T>
    {};

    template <typename T>
    struct is_container<boost::optional<T> >
      : is_container<T>
    {};

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
    template<typename T>
    struct is_container<boost::variant<T> >
      : is_container<T>
    {};

    template<typename T0, typename T1, typename ...TN>
    struct is_container<boost::variant<T0, T1, TN...> >
      : mpl::bool_<is_container<T0>::value ||
            is_container<boost::variant<T1, TN...> >::value>
    {};

#else
#define BOOST_SPIRIT_IS_CONTAINER(z, N, data)                                 \
        is_container<BOOST_PP_CAT(T, N)>::value ||                            \
    /***/

    // make sure unused variant parameters do not affect the outcome
    template <>
    struct is_container<boost::detail::variant::void_>
      : mpl::false_
    {};

    template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct is_container<variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
       : mpl::bool_<BOOST_PP_REPEAT(BOOST_VARIANT_LIMIT_TYPES
            , BOOST_SPIRIT_IS_CONTAINER, _) false>
    {};

#undef BOOST_SPIRIT_IS_CONTAINER
#endif

    template <typename T, typename Enable/* = void*/>
    struct is_iterator_range
      : mpl::false_
    {};

    template <typename T>
    struct is_iterator_range<iterator_range<T> >
      : mpl::true_
    {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        struct remove_value_const
        {
            typedef T type;
        };

        template <typename T>
        struct remove_value_const<T const>
          : remove_value_const<T>
        {};

        template <typename F, typename S>
        struct remove_value_const<std::pair<F, S> >
        {
            typedef typename remove_value_const<F>::type first_type;
            typedef typename remove_value_const<S>::type second_type;
            typedef std::pair<first_type, second_type> type;
        };
    }

    ///////////////////////////////////////////////////////////////////////
    //[customization_container_value_default
    template <typename Container, typename Enable/* = void*/>
    struct container_value
      : detail::remove_value_const<typename Container::value_type>
    {};
    //]

    template <typename T>
    struct container_value<T&>
      : container_value<T>
    {};

    // this will be instantiated if the optional holds a container
    template <typename T>
    struct container_value<boost::optional<T> >
      : container_value<T>
    {};

    // this will be instantiated if the variant holds a container
    template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct container_value<variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    {
        typedef typename
            variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types
        types;
        typedef typename
            mpl::find_if<types, is_container<mpl::_1> >::type
        iter;

        typedef typename container_value<
            typename mpl::if_<
                is_same<iter, typename mpl::end<types>::type>
              , unused_type, typename mpl::deref<iter>::type
            >::type
        >::type type;
    };

    //[customization_container_value_unused
    template <>
    struct container_value<unused_type>
    {
        typedef unused_type type;
    };
    //]

    template <>
    struct container_value<unused_type const>
    {
        typedef unused_type type;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable/* = void*/>
    struct container_iterator
    {
        typedef typename Container::iterator type;
    };

    template <typename Container>
    struct container_iterator<Container&>
      : container_iterator<Container>
    {};

    template <typename Container>
    struct container_iterator<Container const>
    {
        typedef typename Container::const_iterator type;
    };

    template <typename T>
    struct container_iterator<optional<T> >
      : container_iterator<T>
    {};

    template <typename T>
    struct container_iterator<optional<T> const>
      : container_iterator<T const>
    {};

    template <typename Iterator>
    struct container_iterator<iterator_range<Iterator> >
    {
        typedef Iterator type;
    };

    template <>
    struct container_iterator<unused_type>
    {
        typedef unused_type const* type;
    };

    template <>
    struct container_iterator<unused_type const>
    {
        typedef unused_type const* type;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct optional_attribute
    {
        typedef T const& type;

        static type call(T const& val)
        {
            return val;
        }

        static bool is_valid(T const&)
        {
            return true;
        }
    };

    template <typename T>
    struct optional_attribute<boost::optional<T> >
    {
        typedef T const& type;

        static type call(boost::optional<T> const& val)
        {
            return boost::get<T>(val);
        }

        static bool is_valid(boost::optional<T> const& val)
        {
            return !!val;
        }
    };

    template <typename T>
    typename optional_attribute<T>::type
    optional_value(T const& val)
    {
        return optional_attribute<T>::call(val);
    }

    inline unused_type optional_value(unused_type)
    {
        return unused;
    }

    template <typename T>
    bool has_optional_value(T const& val)
    {
        return optional_attribute<T>::is_valid(val);
    }

    inline bool has_optional_value(unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename T>
    bool push_back(Container& c, T const& val);

    //[customization_push_back_default
    template <typename Container, typename T, typename Enable/* = void*/>
    struct push_back_container
    {
        static bool call(Container& c, T const& val)
        {
            c.insert(c.end(), val);
            return true;
        }
    };
    //]

    template <typename Container, typename T>
    struct push_back_container<optional<Container>, T>
    {
        static bool call(boost::optional<Container>& c, T const& val)
        {
            if (!c)
                c = Container();
            return push_back(boost::get<Container>(c), val);
        }
    };

    namespace detail
    {
        template <typename T>
        struct push_back_visitor : public static_visitor<>
        {
            typedef bool result_type;

            push_back_visitor(T const& t) : t_(t) {}

            template <typename Container>
            bool push_back_impl(Container& c, mpl::true_) const
            {
                return push_back(c, t_);
            }

            template <typename T_>
            bool push_back_impl(T_&, mpl::false_) const
            {
                // this variant doesn't hold a container
                BOOST_ASSERT(false && "This variant doesn't hold a container");
                return false;
            }

            template <typename T_>
            bool operator()(T_& c) const
            {
                return push_back_impl(c, typename is_container<T_>::type());
            }

            T const& t_;
        };
    }

    template <BOOST_VARIANT_ENUM_PARAMS(typename T_), typename T>
    struct push_back_container<variant<BOOST_VARIANT_ENUM_PARAMS(T_)>, T>
    {
        static bool call(variant<BOOST_VARIANT_ENUM_PARAMS(T_)>& c, T const& val)
        {
            return apply_visitor(detail::push_back_visitor<T>(val), c);
        }
    };

    template <typename Container, typename T>
    bool push_back(Container& c, T const& val)
    {
        return push_back_container<Container, T>::call(c, val);
    }

    //[customization_push_back_unused
    template <typename Container>
    bool push_back(Container&, unused_type)
    {
        return true;
    }
    //]

    template <typename T>
    bool push_back(unused_type, T const&)
    {
        return true;
    }

    inline bool push_back(unused_type, unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable/* = void*/>
    struct is_empty_container
    {
        static bool call(Container const& c)
        {
            return c.empty();
        }
    };

    template <typename Container>
    bool is_empty(Container const& c)
    {
        return is_empty_container<Container>::call(c);
    }

    inline bool is_empty(unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Ensure the attribute is actually a container type
    template <typename Container, typename Enable/* = void*/>
    struct make_container_attribute
    {
        static void call(Container&)
        {
            // for static types this function does nothing
        }
    };

    template <typename T>
    void make_container(T& t)
    {
        make_container_attribute<T>::call(t);
    }

    inline void make_container(unused_type)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable/* = void*/>
    struct begin_container
    {
        static typename container_iterator<Container>::type call(Container& c)
        {
            return c.begin();
        }
    };

    template <typename Container>
    typename spirit::result_of::begin<Container>::type
    begin(Container& c)
    {
        return begin_container<Container>::call(c);
    }

    inline unused_type const*
    begin(unused_type)
    {
        return &unused;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable/* = void*/>
    struct end_container
    {
        static typename container_iterator<Container>::type call(Container& c)
        {
            return c.end();
        }
    };

    template <typename Container>
    inline typename spirit::result_of::end<Container>::type
    end(Container& c)
    {
        return end_container<Container>::call(c);
    }

    inline unused_type const*
    end(unused_type)
    {
        return &unused;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Enable/* = void*/>
    struct deref_iterator
    {
        typedef typename std::iterator_traits<Iterator>::reference type;
        static type call(Iterator& it)
        {
            return *it;
        }
    };

    template <typename Iterator>
    typename deref_iterator<Iterator>::type
    deref(Iterator& it)
    {
        return deref_iterator<Iterator>::call(it);
    }

    inline unused_type
    deref(unused_type const*)
    {
        return unused;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Enable/* = void*/>
    struct next_iterator
    {
        static void call(Iterator& it)
        {
            ++it;
        }
    };

    template <typename Iterator>
    void next(Iterator& it)
    {
        next_iterator<Iterator>::call(it);
    }

    inline void next(unused_type const*)
    {
        // do nothing
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Enable/* = void*/>
    struct compare_iterators
    {
        static bool call(Iterator const& it1, Iterator const& it2)
        {
            return it1 == it2;
        }
    };

    template <typename Iterator>
    bool compare(Iterator& it1, Iterator& it2)
    {
        return compare_iterators<Iterator>::call(it1, it2);
    }

    inline bool compare(unused_type const*, unused_type const*)
    {
        return false;
    }
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct optional_value
    {
        typedef T type;
    };

    template <typename T>
    struct optional_value<boost::optional<T> >
    {
        typedef T type;
    };

    template <typename T>
    struct optional_value<boost::optional<T> const>
    {
        typedef T const type;
    };

    template <>
    struct optional_value<unused_type>
    {
        typedef unused_type type;
    };

    template <>
    struct optional_value<unused_type const>
    {
        typedef unused_type type;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    struct begin
      : traits::container_iterator<Container>
    {};

    template <typename Container>
    struct end
      : traits::container_iterator<Container>
    {};

    template <typename Iterator>
    struct deref
      : traits::deref_iterator<Iterator>
    {};

    template <>
    struct deref<unused_type const*>
    {
        typedef unused_type type;
    };

}}}

#endif
