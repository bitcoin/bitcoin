//  Boost string_algo library collection_traits.hpp header file  -------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// (C) Copyright Thorsten Ottosen 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// (C) Copyright Jeremy Siek 2001. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Original idea of container traits was proposed by Jeremy Siek and
//  Thorsten Ottosen. This implementation is lightweighted version
//  of container_traits adapter for usage with string_algo library

#ifndef BOOST_RANGE_STRING_COLLECTION_TRAITS_HPP
#define BOOST_RANGE_STRING_COLLECTION_TRAITS_HPP

#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/mpl/eval_if.hpp>

// Implementation
#include <boost/range/detail/collection_traits_detail.hpp>

/*! \file
    Defines collection_traits class and related free-standing functions.
    This facility is used to unify the access to different types of collections.
    It allows the algorithms in the library to work with STL collections, c-style
    array, null-terminated c-strings (and more) using the same interface.
*/

namespace boost {
    namespace algorithm {

//  collection_traits template class -----------------------------------------//
        
        //! collection_traits class
        /*!
            Collection traits provide uniform access to different types of 
            collections. This functionality allows to write generic algorithms
            which work with several different kinds of collections.

            Currently following collection types are supported:
                - containers with STL compatible container interface ( see ContainerConcept )
                    ( i.e. \c std::vector<>, \c std::list<>, \c std::string<> ... )
                - c-style array 
                   ( \c char[10], \c int[15] ... )
                - null-terminated c-strings
                    ( \c char*, \c wchar_T* )
                - std::pair of iterators 
                    ( i.e \c std::pair<vector<int>::iterator,vector<int>::iterator> )

            Collection traits provide an external collection interface operations.
            All are accessible using free-standing functions.

            The following operations are supported:
                - \c size()
                - \c empty()
                - \c begin()
                - \c end()

            Container traits have somewhat limited functionality on compilers not
            supporting partial template specialization and partial template ordering.
        */
        template< typename T >
        struct collection_traits
        {
        private:
            typedef typename ::boost::mpl::eval_if<
                    ::boost::algorithm::detail::is_pair<T>, 
                        detail::pair_container_traits_selector<T>,
                        typename ::boost::mpl::eval_if<
                        ::boost::is_array<T>, 
                            detail::array_container_traits_selector<T>,
                            typename ::boost::mpl::eval_if<
                            ::boost::is_pointer<T>,
                                detail::pointer_container_traits_selector<T>,
                                detail::default_container_traits_selector<T>
                            >
                        > 
                >::type container_helper_type;
        public:
            //! Function type       
            typedef container_helper_type function_type;        
            //! Value type
            typedef typename
                container_helper_type::value_type value_type;
            //! Size type
            typedef typename
                container_helper_type::size_type size_type;
            //! Iterator type
            typedef typename
                container_helper_type::iterator iterator;
            //! Const iterator type
            typedef typename
                container_helper_type::const_iterator const_iterator;
            //! Result iterator type ( iterator of const_iterator, depending on the constness of the container )
            typedef typename
                container_helper_type::result_iterator result_iterator;
            //! Difference type
            typedef typename
                container_helper_type::difference_type difference_type;

        }; // 'collection_traits'

//  collection_traits metafunctions -----------------------------------------//

        //! Container value_type trait
        /*!
            Extract the type of elements contained in a container
        */
        template< typename C >
        struct value_type_of
        {
            typedef typename collection_traits<C>::value_type type;
        };
        
        //! Container difference trait
        /*!
            Extract the container's difference type
        */
        template< typename C >
        struct difference_type_of
        {
            typedef typename collection_traits<C>::difference_type type;
        };

        //! Container iterator trait
        /*!
            Extract the container's iterator type
        */
        template< typename C >
        struct iterator_of
        {
            typedef typename collection_traits<C>::iterator type;
        };

        //! Container const_iterator trait
        /*!
            Extract the container's const_iterator type
        */
        template< typename C >
        struct const_iterator_of
        {
            typedef typename collection_traits<C>::const_iterator type;
        };


        //! Container result_iterator
        /*!
            Extract the container's result_iterator type. This type maps to \c C::iterator
            for mutable container and \c C::const_iterator for const containers.
        */
        template< typename C >
        struct result_iterator_of
        {
            typedef typename collection_traits<C>::result_iterator type;
        };

//  collection_traits related functions -----------------------------------------//

        //! Free-standing size() function
        /*!
            Get the size of the container. Uses collection_traits.
        */
        template< typename C >
        inline typename collection_traits<C>::size_type
        size( const C& c )
        {
            return collection_traits<C>::function_type::size( c ); 
        }

        //! Free-standing empty() function
        /*!
            Check whether the container is empty. Uses container traits.
        */
        template< typename C >
        inline bool empty( const C& c )
        {
            return collection_traits<C>::function_type::empty( c );
        }

        //! Free-standing begin() function
        /*!
            Get the begin iterator of the container. Uses collection_traits.
        */
        template< typename C >
        inline typename collection_traits<C>::iterator
        begin( C& c )
        {
            return collection_traits<C>::function_type::begin( c ); 
        }

        //! Free-standing begin() function
        /*!
            \overload
        */
        template< typename C >
        inline typename collection_traits<C>::const_iterator
        begin( const C& c )
        {
            return collection_traits<C>::function_type::begin( c ); 
        }

        //! Free-standing end() function
        /*!
            Get the begin iterator of the container. Uses collection_traits.
        */
        template< typename C >
        inline typename collection_traits<C>::iterator
        end( C& c )
        {
            return collection_traits<C>::function_type::end( c );
        }

        //! Free-standing end() function
        /*!
            \overload           
        */
        template< typename C >
        inline typename collection_traits<C>::const_iterator
        end( const C& c )
        {
            return collection_traits<C>::function_type::end( c );
        }

    } // namespace algorithm
} // namespace boost

#endif // BOOST_STRING_COLLECTION_TRAITS_HPP
