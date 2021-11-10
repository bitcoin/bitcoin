/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_CONTAINER_FEBRUARY_06_2007_1001AM)
#define BOOST_SPIRIT_X3_CONTAINER_FEBRUARY_06_2007_1001AM

#include <boost/fusion/support/category_of.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/fusion/include/deque.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/make_void.hpp>

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  This file contains some container utils for stl containers.
    ///////////////////////////////////////////////////////////////////////////

    namespace detail
    {
        template <typename T, typename Enabler = void>
        struct is_container_impl : mpl::false_ {};

        template <typename T>
        struct is_container_impl<T, void_t<
            typename T::value_type, typename T::iterator,
            typename T::size_type, typename T::reference> > : mpl::true_ {};

        template <typename T, typename Enabler = void>
        struct is_associative_impl : mpl::false_ {};

        template <typename T>
        struct is_associative_impl<T, void_t<typename T::key_type>>
            : mpl::true_ {};
    }

    template <typename T>
    using is_container = typename detail::is_container_impl<T>::type;

    template <typename T>
    using is_associative = typename detail::is_associative_impl<T>::type;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        struct remove_value_const : mpl::identity<T> {};

        template <typename T>
        struct remove_value_const<T const> : remove_value_const<T> {};

        template <typename F, typename S>
        struct remove_value_const<std::pair<F, S>>
        {
            typedef typename remove_value_const<F>::type first_type;
            typedef typename remove_value_const<S>::type second_type;
            typedef std::pair<first_type, second_type> type;
        };
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct container_value
      : detail::remove_value_const<typename Container::value_type>
    {};

    template <typename Container>
    struct container_value<Container const> : container_value<Container> {};

    // There is no single container value for fusion maps, but because output
    // of this metafunc is used to check wheter parser's attribute can be
    // saved to container, we simply return whole fusion::map as is
    // so that check can be done in traits::is_substitute specialisation
    template <typename T>
    struct container_value<T
			   , typename enable_if<typename mpl::eval_if <
						    fusion::traits::is_sequence<T>
						    , fusion::traits::is_associative<T>
						    , mpl::false_ >::type >::type>
    : mpl::identity<T> {};

    template <>
    struct container_value<unused_type> : mpl::identity<unused_type> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct container_iterator
        : mpl::identity<typename Container::iterator> {};

    template <typename Container>
    struct container_iterator<Container const>
         : mpl::identity<typename Container::const_iterator> {};

    template <>
    struct container_iterator<unused_type>
        : mpl::identity<unused_type const*> {};

    template <>
    struct container_iterator<unused_type const>
        : mpl::identity<unused_type const*> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename T>
    bool push_back(Container& c, T&& val);

    template <typename Container, typename Enable = void>
    struct push_back_container
    {
        template <typename T>
        static bool call(Container& c, T&& val)
        {
            c.insert(c.end(), static_cast<T&&>(val));
            return true;
        }
    };

    template <typename Container, typename T>
    inline bool push_back(Container& c, T&& val)
    {
        return push_back_container<Container>::call(c, static_cast<T&&>(val));
    }

    template <typename Container>
    inline bool push_back(Container&, unused_type)
    {
        return true;
    }

    template <typename T>
    inline bool push_back(unused_type, T&&)
    {
        return true;
    }

    inline bool push_back(unused_type, unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Iterator>
    bool append(Container& c, Iterator first, Iterator last);

    template <typename Container, typename Enable = void>
    struct append_container
    {
    private:
        template <typename Iterator>
        static void insert(Container& c, Iterator first, Iterator last, mpl::false_)
        {
            c.insert(c.end(), first, last);
        }

        template <typename Iterator>
        static void insert(Container& c, Iterator first, Iterator last, mpl::true_)
        {
            c.insert(first, last);
        }

    public:
        template <typename Iterator>
        static bool call(Container& c, Iterator first, Iterator last)
        {
            insert(c, first, last, is_associative<Container>{});
            return true;
        }
    };

    template <typename Container, typename Iterator>
    inline bool append(Container& c, Iterator first, Iterator last)
    {
        return append_container<Container>::call(c, first, last);
    }

    template <typename Iterator>
    inline bool append(unused_type, Iterator /* first */, Iterator /* last */)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct is_empty_container
    {
        static bool call(Container const& c)
        {
            return c.empty();
        }
    };

    template <typename Container>
    inline bool is_empty(Container const& c)
    {
        return is_empty_container<Container>::call(c);
    }

    inline bool is_empty(unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container, typename Enable = void>
    struct begin_container
    {
        static typename container_iterator<Container>::type call(Container& c)
        {
            return c.begin();
        }
    };

    template <typename Container>
    inline typename container_iterator<Container>::type
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
    template <typename Container, typename Enable = void>
    struct end_container
    {
        static typename container_iterator<Container>::type call(Container& c)
        {
            return c.end();
        }
    };

    template <typename Container>
    inline typename container_iterator<Container>::type
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
    template <typename Iterator, typename Enable = void>
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
    template <typename Iterator, typename Enable = void>
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
    template <typename Iterator, typename Enable = void>
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

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct build_container : mpl::identity<std::vector<T>> {};

    template <typename T>
    struct build_container<boost::fusion::deque<T> > : build_container<T> {};

    template <>
    struct build_container<unused_type> : mpl::identity<unused_type> {};

    template <>
    struct build_container<char> : mpl::identity<std::string> {};

}}}}

#endif
