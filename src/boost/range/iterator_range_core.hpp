// Boost.Range library
//
//  Copyright Neil Groves & Thorsten Ottosen & Pavol Droba 2003-2004.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
// Credits:
// 'michel' reported Trac 9072 which included a patch for allowing references
// to function types.
//
#ifndef BOOST_RANGE_ITERATOR_RANGE_CORE_HPP_INCLUDED
#define BOOST_RANGE_ITERATOR_RANGE_CORE_HPP_INCLUDED

#include <boost/config.hpp> // Define __STL_CONFIG_H, if appropriate.
#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
    #pragma warning( push )
    #pragma warning( disable : 4996 )
#endif

#include <boost/assert.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/has_range_iterator.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/detail/safe_bool.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/next_prior.hpp>
#include <iterator>
#include <algorithm>
#include <cstddef>

/*! \file
    Defines the \c iterator_class and related functions.
    \c iterator_range is a simple wrapper of iterator pair idiom. It provides
    a rich subset of Container interface.
*/


namespace boost
{
    namespace iterator_range_detail
    {
        //
        // The functions adl_begin and adl_end are implemented in a separate
        // class for gcc-2.9x
        //
        template<class IteratorT>
        struct iterator_range_impl {
            template< class ForwardRange >
            static IteratorT adl_begin( ForwardRange& r )
            {
                return IteratorT( boost::begin( r ) );
            }

            template< class ForwardRange >
            static IteratorT adl_end( ForwardRange& r )
            {
                return IteratorT( boost::end( r ) );
            }
        };

        template< class Left, class Right >
        inline bool less_than( const Left& l, const Right& r )
        {
            return std::lexicographical_compare( boost::begin(l),
                                                 boost::end(l),
                                                 boost::begin(r),
                                                 boost::end(r) );
        }
        
        template< class Left, class Right >
        inline bool greater_than( const Left& l, const Right& r )
        {
            return iterator_range_detail::less_than(r,l);
        }
        
        template< class Left, class Right >
        inline bool less_or_equal_than( const Left& l, const Right& r )
        {
            return !iterator_range_detail::less_than(r,l);
        }
        
        template< class Left, class Right >
        inline bool greater_or_equal_than( const Left& l, const Right& r )
        {
            return !iterator_range_detail::less_than(l,r);
        }

        // This version is maintained since it is used in other boost libraries
        // such as Boost.Assign
        template< class Left, class Right >
        inline bool equal(const Left& l, const Right& r)
        {
            return boost::equal(l, r);
        }

struct range_tag
{
};

struct const_range_tag
{
};

struct iterator_range_tag
{
};

typedef char (&incrementable_t)[1];
typedef char (&bidirectional_t)[2];
typedef char (&random_access_t)[3];

incrementable_t test_traversal_tag(boost::incrementable_traversal_tag);
bidirectional_t test_traversal_tag(boost::bidirectional_traversal_tag);
random_access_t test_traversal_tag(boost::random_access_traversal_tag);

template<std::size_t S>
struct pure_iterator_traversal_impl
{
    typedef boost::incrementable_traversal_tag type;
};

template<>
struct pure_iterator_traversal_impl<sizeof(bidirectional_t)>
{
    typedef boost::bidirectional_traversal_tag type;
};

template<>
struct pure_iterator_traversal_impl<sizeof(random_access_t)>
{
    typedef boost::random_access_traversal_tag type;
};

template<typename IteratorT>
struct pure_iterator_traversal
{
    typedef
        BOOST_DEDUCED_TYPENAME iterator_traversal<IteratorT>::type
    traversal_t;
    BOOST_STATIC_CONSTANT(
        std::size_t,
        traversal_i = sizeof(iterator_range_detail::test_traversal_tag((traversal_t())))
    );
    typedef
        BOOST_DEDUCED_TYPENAME pure_iterator_traversal_impl<traversal_i>::type
    type;
};

template<class IteratorT, class TraversalTag>
class iterator_range_base
    : public iterator_range_tag
{
    typedef range_detail::safe_bool<
                IteratorT iterator_range_base<IteratorT, TraversalTag>::*
    > safe_bool_t;

    typedef iterator_range_base<IteratorT, TraversalTag> type;

protected:
    typedef iterator_range_impl<IteratorT> impl;

public:
    typedef BOOST_DEDUCED_TYPENAME
        safe_bool_t::unspecified_bool_type unspecified_bool_type;

    typedef BOOST_DEDUCED_TYPENAME
        iterator_value<IteratorT>::type value_type;

    typedef BOOST_DEDUCED_TYPENAME
        iterator_difference<IteratorT>::type difference_type;

    typedef std::size_t size_type; // note: must be unsigned

    // Needed because value-type is the same for
    // const and non-const iterators
    typedef BOOST_DEDUCED_TYPENAME
                iterator_reference<IteratorT>::type reference;

    //! const_iterator type
    /*!
        There is no distinction between const_iterator and iterator.
        These typedefs are provides to fulfill container interface
    */
    typedef IteratorT const_iterator;
    //! iterator type
    typedef IteratorT iterator;

protected:
    iterator_range_base()
        : m_Begin()
        , m_End()
    {
    }

    template<class Iterator>
    iterator_range_base(Iterator Begin, Iterator End)
        : m_Begin(Begin)
        , m_End(End)
    {
    }

public:
    IteratorT begin() const
    {
        return m_Begin;
    }

    IteratorT end() const
    {
        return m_End;
    }

    bool empty() const
    {
        return m_Begin == m_End;
    }

    operator unspecified_bool_type() const
    {
        return safe_bool_t::to_unspecified_bool(
                    m_Begin != m_End, &iterator_range_base::m_Begin);
    }

    bool operator!() const
    {
        return empty();
    }

    bool equal(const iterator_range_base& r) const
    {
        return m_Begin == r.m_Begin && m_End == r.m_End;
    }

   reference front() const
   {
       BOOST_ASSERT(!empty());
       return *m_Begin;
   }

   void drop_front()
   {
       BOOST_ASSERT(!empty());
       ++m_Begin;
   }

   void drop_front(difference_type n)
   {
       BOOST_ASSERT(n >= difference_type());
       std::advance(this->m_Begin, n);
   }
   
   // Deprecated
   void pop_front() { drop_front(); }

protected:
    template<class Iterator>
    void assign(Iterator first, Iterator last)
    {
        m_Begin = first;
        m_End = last;
    }

    template<class SinglePassRange>
    void assign(const SinglePassRange& r)
    {
        m_Begin = impl::adl_begin(r);
        m_End = impl::adl_end(r);
    }

    template<class SinglePassRange>
    void assign(SinglePassRange& r)
    {
        m_Begin = impl::adl_begin(r);
        m_End = impl::adl_end(r);
    }

    IteratorT m_Begin;
    IteratorT m_End;
};

template<class IteratorT>
class iterator_range_base<IteratorT, bidirectional_traversal_tag>
        : public iterator_range_base<IteratorT, incrementable_traversal_tag>
{
    typedef iterator_range_base<IteratorT, incrementable_traversal_tag> base_type;

protected:
    iterator_range_base()
    {
    }

    template<class Iterator>
    iterator_range_base(Iterator first, Iterator last)
        : base_type(first, last)
    {
    }

public:
    typedef BOOST_DEDUCED_TYPENAME base_type::difference_type difference_type;
    typedef BOOST_DEDUCED_TYPENAME base_type::reference reference;

    reference back() const
    {
        BOOST_ASSERT(!this->empty());
        return *boost::prior(this->m_End);
    }

    void drop_back()
    {
        BOOST_ASSERT(!this->empty());
        --this->m_End;
    }

    void drop_back(difference_type n)
    {
        BOOST_ASSERT(n >= difference_type());
        std::advance(this->m_End, -n);
    }
    
    // Deprecated
    void pop_back() { drop_back(); }
};

template<class IteratorT>
class iterator_range_base<IteratorT, random_access_traversal_tag>
        : public iterator_range_base<IteratorT, bidirectional_traversal_tag>
{
    typedef iterator_range_base<
                IteratorT, bidirectional_traversal_tag> base_type;

public:
    typedef BOOST_DEDUCED_TYPENAME
        boost::mpl::if_<
            boost::mpl::or_<
                boost::is_abstract<
                    BOOST_DEDUCED_TYPENAME base_type::value_type
                >,
                boost::is_array<
                    BOOST_DEDUCED_TYPENAME base_type::value_type
                >,
                boost::is_function<
                    BOOST_DEDUCED_TYPENAME base_type::value_type
                >
            >,
            BOOST_DEDUCED_TYPENAME base_type::reference,
            BOOST_DEDUCED_TYPENAME base_type::value_type
        >::type abstract_value_type;

    // Rationale:
    // typedef these here to reduce verbiage in the implementation of this
    // type.
    typedef BOOST_DEDUCED_TYPENAME base_type::difference_type difference_type;
    typedef BOOST_DEDUCED_TYPENAME base_type::size_type size_type;
    typedef BOOST_DEDUCED_TYPENAME base_type::reference reference;

protected:
    iterator_range_base()
    {
    }

    template<class Iterator>
    iterator_range_base(Iterator first, Iterator last)
        : base_type(first, last)
    {
    }

public:
    reference operator[](difference_type at) const
    {
        BOOST_ASSERT(at >= 0);
        BOOST_ASSERT(static_cast<typename base_type::size_type>(at) < size());
        return this->m_Begin[at];
    }

    //
    // When storing transform iterators, operator[]()
    // fails because it returns by reference. Therefore
    // operator()() is provided for these cases.
    //
    abstract_value_type operator()(difference_type at) const
    {
        BOOST_ASSERT(at >= 0);
        BOOST_ASSERT(static_cast<typename base_type::size_type>(at) < size());
        return this->m_Begin[at];
    }

    BOOST_DEDUCED_TYPENAME base_type::size_type size() const
    {
        return this->m_End - this->m_Begin;
    }
};

    }

//  iterator range template class -----------------------------------------//

        //! iterator_range class
        /*!
            An \c iterator_range delimits a range in a sequence by beginning and ending iterators.
            An iterator_range can be passed to an algorithm which requires a sequence as an input.
            For example, the \c toupper() function may be used most frequently on strings,
            but can also be used on iterator_ranges:

            \code
                boost::tolower( find( s, "UPPERCASE STRING" ) );
            \endcode

            Many algorithms working with sequences take a pair of iterators,
            delimiting a working range, as an arguments. The \c iterator_range class is an
            encapsulation of a range identified by a pair of iterators.
            It provides a collection interface,
            so it is possible to pass an instance to an algorithm requiring a collection as an input.
        */
        template<class IteratorT>
        class iterator_range
            : public iterator_range_detail::iterator_range_base<
                    IteratorT,
                    BOOST_DEDUCED_TYPENAME iterator_range_detail::pure_iterator_traversal<IteratorT>::type
                >
        {
            typedef iterator_range_detail::iterator_range_base<
                    IteratorT,
                    BOOST_DEDUCED_TYPENAME iterator_range_detail::pure_iterator_traversal<IteratorT>::type
            > base_type;

            template<class Source>
            struct is_compatible_range_
              : is_convertible<
                    BOOST_DEDUCED_TYPENAME mpl::eval_if<
                        has_range_iterator<Source>,
                        range_iterator<Source>,
                        mpl::identity<void>
                    >::type,
                    BOOST_DEDUCED_TYPENAME base_type::iterator
                >
            {
            };

            template<class Source>
            struct is_compatible_range
                : mpl::and_<
                    mpl::not_<
                        is_convertible<
                            Source,
                            BOOST_DEDUCED_TYPENAME base_type::iterator
                        >
                    >,
                    is_compatible_range_<Source>
                >
            {
            };

        protected:
            typedef iterator_range_detail::iterator_range_impl<IteratorT> impl;

        public:
            typedef iterator_range<IteratorT> type;

            iterator_range()
            {
            }

            template<class Iterator>
            iterator_range(Iterator first, Iterator last)
                : base_type(first, last)
            {
            }

            template<class SinglePassRange>
            iterator_range(
                const SinglePassRange& r,
                BOOST_DEDUCED_TYPENAME ::boost::enable_if<
                    is_compatible_range<const SinglePassRange>
                >::type* = 0
            )
                : base_type(impl::adl_begin(r), impl::adl_end(r))
            {
            }

            template<class SinglePassRange>
            iterator_range(
                SinglePassRange& r,
                BOOST_DEDUCED_TYPENAME ::boost::enable_if<
                    is_compatible_range<SinglePassRange>
                >::type* = 0
            )
                : base_type(impl::adl_begin(r), impl::adl_end(r))
            {
            }

            template<class SinglePassRange>
            iterator_range(const SinglePassRange& r,
                           iterator_range_detail::const_range_tag)
                : base_type(impl::adl_begin(r), impl::adl_end(r))
            {
            }

            template<class SinglePassRange>
            iterator_range(SinglePassRange& r,
                           iterator_range_detail::range_tag)
                : base_type(impl::adl_begin(r), impl::adl_end(r))
            {
            }

            template<class Iterator>
            iterator_range& operator=(const iterator_range<Iterator>& other)
            {
                this->assign(other.begin(), other.end());
                return *this;
            }

            template<class Iterator>
            iterator_range& operator=(iterator_range<Iterator>& other)
            {
                this->assign(other.begin(), other.end());
                return *this;
            }

            template<class SinglePassRange>
            iterator_range& operator=(SinglePassRange& r)
            {
                this->assign(r);
                return *this;
            }

            template<class SinglePassRange>
            iterator_range& operator=(const SinglePassRange& r)
            {
                this->assign(r);
                return *this;
            }

            iterator_range& advance_begin(
                BOOST_DEDUCED_TYPENAME base_type::difference_type n)
            {
                std::advance(this->m_Begin, n);
                return *this;
            }

            iterator_range& advance_end(
                BOOST_DEDUCED_TYPENAME base_type::difference_type n)
            {
                std::advance(this->m_End, n);
                return *this;
            }

        protected:
            //
            // Allow subclasses an easy way to access the
            // base type
            //
            typedef iterator_range iterator_range_;
        };

//  iterator range free-standing operators ---------------------------//

        /////////////////////////////////////////////////////////////////////
        // comparison operators
        /////////////////////////////////////////////////////////////////////

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator==( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return boost::equal( l, r );
        }

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator!=( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return !boost::equal( l, r );
        }

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator<( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return iterator_range_detail::less_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator<=( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return iterator_range_detail::less_or_equal_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator>( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return iterator_range_detail::greater_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator>=( const ForwardRange& l, const iterator_range<IteratorT>& r )
        {
            return iterator_range_detail::greater_or_equal_than( l, r );
        }

        template< class Iterator1T, class Iterator2T >
        inline bool
        operator==( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return boost::equal( l, r );
        }

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator==( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return boost::equal( l, r );
        }


        template< class Iterator1T, class Iterator2T >
        inline bool
        operator!=( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return !boost::equal( l, r );
        }

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator!=( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return !boost::equal( l, r );
        }


        template< class Iterator1T, class Iterator2T >
        inline bool
        operator<( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return iterator_range_detail::less_than( l, r );
        }

        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator<( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return iterator_range_detail::less_than( l, r );
        }
        
        template< class Iterator1T, class Iterator2T >
        inline bool
        operator<=( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return iterator_range_detail::less_or_equal_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator<=( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return iterator_range_detail::less_or_equal_than( l, r );
        }
        
        template< class Iterator1T, class Iterator2T >
        inline bool
        operator>( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return iterator_range_detail::greater_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator>( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return iterator_range_detail::greater_than( l, r );
        }
        
        template< class Iterator1T, class Iterator2T >
        inline bool
        operator>=( const iterator_range<Iterator1T>& l, const iterator_range<Iterator2T>& r )
        {
            return iterator_range_detail::greater_or_equal_than( l, r );
        }
        
        template< class IteratorT, class ForwardRange >
        inline BOOST_DEDUCED_TYPENAME boost::enable_if<
            mpl::not_<boost::is_base_and_derived<iterator_range_detail::iterator_range_tag, ForwardRange> >,
            bool
        >::type
        operator>=( const iterator_range<IteratorT>& l, const ForwardRange& r )
        {
            return iterator_range_detail::greater_or_equal_than( l, r );
        }

//  iterator range utilities -----------------------------------------//

        //! iterator_range construct helper
        /*!
            Construct an \c iterator_range from a pair of iterators

            \param Begin A begin iterator
            \param End An end iterator
            \return iterator_range object
        */
        template< typename IteratorT >
        inline iterator_range< IteratorT >
        make_iterator_range( IteratorT Begin, IteratorT End )
        {
            return iterator_range<IteratorT>( Begin, End );
        }

        template<typename IteratorT, typename IntegerT>
        inline iterator_range<IteratorT>
        make_iterator_range_n(IteratorT first, IntegerT n)
        {
            return iterator_range<IteratorT>(first, boost::next(first, n));
        }

        //! iterator_range construct helper
        /*!
            Construct an \c iterator_range from a \c Range containing the begin
            and end iterators.
        */
        template< class ForwardRange >
        inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type >
        make_iterator_range( ForwardRange& r )
        {
           return iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type >
                ( r, iterator_range_detail::range_tag() );
        }

        template< class ForwardRange >
        inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type >
        make_iterator_range( const ForwardRange& r )
        {
           return iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type >
                ( r, iterator_range_detail::const_range_tag() );
        }

        namespace iterator_range_detail
        {
            template< class Range >
            inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<Range>::type >
            make_range_impl( Range& r,
                             BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_begin,
                             BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_end )
            {
                //
                // Not worth the effort
                //
                //if( advance_begin == 0 && advance_end == 0 )
                //    return make_iterator_range( r );
                //

                BOOST_DEDUCED_TYPENAME range_iterator<Range>::type
                    new_begin = boost::begin( r ),
                    new_end   = boost::end( r );
                std::advance( new_begin, advance_begin );
                std::advance( new_end, advance_end );
                return make_iterator_range( new_begin, new_end );
            }
        }

        template< class Range >
        inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<Range>::type >
        make_iterator_range( Range& r,
                    BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_begin,
                    BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_end )
        {
            return iterator_range_detail::make_range_impl( r, advance_begin, advance_end );
        }

        template< class Range >
        inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<const Range>::type >
        make_iterator_range( const Range& r,
                    BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_begin,
                    BOOST_DEDUCED_TYPENAME range_difference<Range>::type advance_end )
        {
            return iterator_range_detail::make_range_impl( r, advance_begin, advance_end );
        }

        //! copy a range into a sequence
        /*!
            Construct a new sequence of the specified type from the elements
            in the given range

            \param Range An input range
            \return New sequence
        */
        template< typename SeqT, typename Range >
        inline SeqT copy_range( const Range& r )
        {
            return SeqT( boost::begin( r ), boost::end( r ) );
        }

} // namespace 'boost'

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
    #pragma warning( pop )
#endif

#endif

