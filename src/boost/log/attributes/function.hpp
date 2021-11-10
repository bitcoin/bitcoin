/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   function.hpp
 * \author Andrey Semashev
 * \date   24.06.2007
 *
 * The header contains implementation of an attribute that calls a third-party function on value acquisition.
 */

#ifndef BOOST_LOG_ATTRIBUTES_FUNCTION_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_FUNCTION_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that acquires its value from a third-party function object
 *
 * The attribute calls a stored nullary function object to acquire each value.
 * The result type of the function object is the attribute value type.
 *
 * It is not recommended to use this class directly. Use \c make_function convenience functions
 * to construct the attribute instead.
 */
template< typename R >
class function :
    public attribute
{
    BOOST_STATIC_ASSERT_MSG(!is_void< R >::value, "Boost.Log: Function object return type must not be void");

public:
    //! The attribute value type
    typedef R value_type;

protected:
    //! Base class for factory implementation
    class BOOST_LOG_NO_VTABLE BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
    };

    //! Factory implementation
    template< typename T >
    class impl_template :
        public impl
    {
    private:
        //! Functor that returns attribute values
        /*!
         * \note The constness signifies that the function object should avoid
         *       modifying its state since it's not protected against concurrent calls.
         */
        const T m_Functor;

    public:
        /*!
         * Constructor with the stored delegate initialization
         */
        explicit impl_template(T const& fun) : m_Functor(fun) {}

        attribute_value get_value()
        {
            return attributes::make_attribute_value(m_Functor());
        }
    };

public:
    /*!
     * Initializing constructor
     */
    template< typename T >
    explicit function(T const& fun) : attribute(new impl_template< T >(fun))
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit function(cast_source const& source) :
        attribute(source.as< impl >())
    {
    }
};

#ifndef BOOST_NO_RESULT_OF

/*!
 * The function constructs \c function attribute instance with the provided function object.
 *
 * \param fun Nullary functional object that returns an actual stored value for an attribute value.
 * \return Pointer to the attribute instance
 */
template< typename T >
inline function<
    typename remove_cv<
        typename remove_reference<
            typename boost::result_of< T() >::type
        >::type
    >::type
> make_function(T const& fun)
{
    typedef typename remove_cv<
        typename remove_reference<
            typename boost::result_of< T() >::type
        >::type
    >::type result_type;

    typedef function< result_type > function_type;
    return function_type(fun);
}

#endif // BOOST_NO_RESULT_OF

#ifndef BOOST_LOG_DOXYGEN_PASS

/*!
 * The function constructs \c function attribute instance with the provided function object.
 * Use this version if your compiler fails to determine the result type of the function object.
 *
 * \param fun Nullary functional object that returns an actual stored value for an attribute value.
 * \return Pointer to the attribute instance
 */
template< typename R, typename T >
inline function<
    typename remove_cv<
        typename remove_reference< R >::type
    >::type
> make_function(T const& fun)
{
    typedef typename remove_cv<
        typename remove_reference< R >::type
    >::type result_type;

    typedef function< result_type > function_type;
    return function_type(fun);
}

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_FUNCTOR_HPP_INCLUDED_
