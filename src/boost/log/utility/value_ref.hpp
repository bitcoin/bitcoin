/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   value_ref.hpp
 * \author Andrey Semashev
 * \date   27.07.2012
 *
 * The header contains implementation of a value reference wrapper.
 */

#ifndef BOOST_LOG_UTILITY_VALUE_REF_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_VALUE_REF_HPP_INCLUDED_

#include <cstddef>
#include <iosfwd>
#include <boost/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/index_of.hpp>
#include <boost/core/explicit_operator_bool.hpp>
#include <boost/core/addressof.hpp>
#include <boost/optional/optional_fwd.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/detail/value_ref_visitation.hpp>
#include <boost/log/detail/sfinae_tools.hpp>
#include <boost/log/utility/formatting_ostream_fwd.hpp>
#include <boost/log/utility/functional/logical.hpp>
#include <boost/log/utility/functional/bind.hpp>
#include <boost/log/utility/functional/bind_output.hpp>
#include <boost/log/utility/functional/bind_to_log.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/utility/value_ref_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The function object applies the function object to the bound visitable object and argument
template< typename VisitableT, typename FunT >
struct vistation_invoker
{
    typedef typename FunT::result_type result_type;

    vistation_invoker(VisitableT& visitable, result_type const& def_val) : m_visitable(visitable), m_def_val(def_val)
    {
    }

    template< typename ArgT >
    result_type operator() (ArgT const& arg) const
    {
        return m_visitable.apply_visitor_or_default(binder1st< FunT, ArgT const& >(FunT(), arg), m_def_val);
    }

private:
    VisitableT& m_visitable;
    result_type m_def_val;
};

//! Traits for testing type compatibility with the reference wrapper
struct singular_ref_compatibility_traits
{
    template< typename T, typename U >
    struct is_compatible
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };
    template< typename T >
    struct is_compatible< T, T >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
};

//! Attribute value reference implementation for a single type case
template< typename T, typename TagT >
class singular_ref
{
public:
    //! Referenced value type
    typedef T value_type;
    //! Tag type
    typedef TagT tag_type;

protected:
    //! Traits for testing type compatibility with the reference wrapper
    typedef singular_ref_compatibility_traits compatibility_traits;

protected:
    //! Pointer to the value
    const value_type* m_ptr;

protected:
    //! Default constructor
    singular_ref() BOOST_NOEXCEPT : m_ptr(NULL)
    {
    }

    //! Initializing constructor
    explicit singular_ref(const value_type* p) BOOST_NOEXCEPT : m_ptr(p)
    {
    }

public:
    //! Returns a pointer to the referred value
    const value_type* operator-> () const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_ptr != NULL);
        return m_ptr;
    }

    //! Returns a pointer to the referred value
    const value_type* get_ptr() const BOOST_NOEXCEPT
    {
        return m_ptr;
    }

    //! Returns a pointer to the referred value
    template< typename U >
    typename boost::enable_if_c< compatibility_traits::is_compatible< value_type, U >::value, const U* >::type get_ptr() const BOOST_NOEXCEPT
    {
        return m_ptr;
    }

    //! Returns a reference to the value
    value_type const& operator* () const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    //! Returns a reference to the value
    value_type const& get() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    //! Returns a reference to the value
    template< typename U >
    typename boost::enable_if_c< compatibility_traits::is_compatible< value_type, U >::value, U const& >::type get() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_ptr != NULL);
        return *m_ptr;
    }


    //! Resets the reference
    void reset() BOOST_NOEXCEPT
    {
        m_ptr = NULL;
    }

    //! Returns the stored type index
    static BOOST_CONSTEXPR unsigned int which()
    {
        return 0u;
    }

    //! Swaps two reference wrappers
    void swap(singular_ref& that) BOOST_NOEXCEPT
    {
        const void* p = m_ptr;
        m_ptr = that.m_ptr;
        that.m_ptr = p;
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename VisitorT::result_type apply_visitor(VisitorT visitor) const
    {
        BOOST_ASSERT(m_ptr != NULL);
        return visitor(*m_ptr);
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename boost::enable_if_c< is_void< typename VisitorT::result_type >::value, bool >::type apply_visitor_optional(VisitorT visitor) const
    {
        if (m_ptr)
        {
            visitor(*m_ptr);
            return true;
        }
        else
            return false;
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename boost::disable_if_c< is_void< typename VisitorT::result_type >::value, optional< typename VisitorT::result_type > >::type apply_visitor_optional(VisitorT visitor) const
    {
        typedef optional< typename VisitorT::result_type > result_type;
        if (m_ptr)
            return result_type(visitor(*m_ptr));
        else
            return result_type();
    }

    //! Applies a visitor function object to the referred value or returns a default value
    template< typename VisitorT, typename DefaultT >
    typename VisitorT::result_type apply_visitor_or_default(VisitorT visitor, DefaultT& def_val) const
    {
        if (m_ptr)
            return visitor(*m_ptr);
        else
            return def_val;
    }

    //! Applies a visitor function object to the referred value or returns a default value
    template< typename VisitorT, typename DefaultT >
    typename VisitorT::result_type apply_visitor_or_default(VisitorT visitor, DefaultT const& def_val) const
    {
        if (m_ptr)
            return visitor(*m_ptr);
        else
            return def_val;
    }
};

//! Traits for testing type compatibility with the reference wrapper
struct variant_ref_compatibility_traits
{
    template< typename T, typename U >
    struct is_compatible
    {
        BOOST_STATIC_CONSTANT(bool, value = (mpl::contains< T, U >::type::value));
    };
};

//! Attribute value reference implementation for multiple types case
template< typename T, typename TagT >
class variant_ref
{
public:
    //! Referenced value type
    typedef T value_type;
    //! Tag type
    typedef TagT tag_type;

protected:
    //! Traits for testing type compatibility with the reference wrapper
    typedef variant_ref_compatibility_traits compatibility_traits;

protected:
    //! Pointer to the value
    const void* m_ptr;
    //! Type index
    unsigned int m_type_idx;

protected:
    //! Default constructor
    variant_ref() BOOST_NOEXCEPT : m_ptr(NULL), m_type_idx(0)
    {
    }

    //! Initializing constructor
    template< typename U >
    explicit variant_ref(const U* p) BOOST_NOEXCEPT : m_ptr(p), m_type_idx(mpl::index_of< value_type, U >::type::value)
    {
    }

public:
    //! Resets the reference
    void reset() BOOST_NOEXCEPT
    {
        m_ptr = NULL;
        m_type_idx = 0;
    }

    //! Returns a pointer to the referred value
    template< typename U >
    typename boost::enable_if_c< compatibility_traits::is_compatible< value_type, U >::value, const U* >::type get_ptr() const BOOST_NOEXCEPT
    {
        if (m_type_idx == static_cast< unsigned int >(mpl::index_of< value_type, U >::type::value))
            return static_cast< const U* >(m_ptr);
        else
            return NULL;
    }

    //! Returns a reference to the value
    template< typename U >
    typename boost::enable_if_c< compatibility_traits::is_compatible< value_type, U >::value, U const& >::type get() const BOOST_NOEXCEPT
    {
        const U* const p = get_ptr< U >();
        BOOST_ASSERT(p != NULL);
        return *p;
    }

    //! Returns the stored type index
    unsigned int which() const BOOST_NOEXCEPT
    {
        return m_type_idx;
    }

    //! Swaps two reference wrappers
    void swap(variant_ref& that) BOOST_NOEXCEPT
    {
        const void* p = m_ptr;
        m_ptr = that.m_ptr;
        that.m_ptr = p;
        unsigned int type_idx = m_type_idx;
        m_type_idx = that.m_type_idx;
        that.m_type_idx = type_idx;
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename VisitorT::result_type apply_visitor(VisitorT visitor) const
    {
        BOOST_ASSERT(m_ptr != NULL);
        return do_apply_visitor(visitor);
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename boost::enable_if_c< is_void< typename VisitorT::result_type >::value, bool >::type apply_visitor_optional(VisitorT visitor) const
    {
        if (m_ptr)
        {
            do_apply_visitor(visitor);
            return true;
        }
        else
            return false;
    }

    //! Applies a visitor function object to the referred value
    template< typename VisitorT >
    typename boost::disable_if_c< is_void< typename VisitorT::result_type >::value, optional< typename VisitorT::result_type > >::type apply_visitor_optional(VisitorT visitor) const
    {
        typedef optional< typename VisitorT::result_type > result_type;
        if (m_ptr)
            return result_type(do_apply_visitor(visitor));
        else
            return result_type();
    }

    //! Applies a visitor function object to the referred value or returns a default value
    template< typename VisitorT, typename DefaultT >
    typename VisitorT::result_type apply_visitor_or_default(VisitorT visitor, DefaultT& def_val) const
    {
        if (m_ptr)
            return do_apply_visitor(visitor);
        else
            return def_val;
    }

    //! Applies a visitor function object to the referred value or returns a default value
    template< typename VisitorT, typename DefaultT >
    typename VisitorT::result_type apply_visitor_or_default(VisitorT visitor, DefaultT const& def_val) const
    {
        if (m_ptr)
            return do_apply_visitor(visitor);
        else
            return def_val;
    }

private:
    template< typename VisitorT >
    typename VisitorT::result_type do_apply_visitor(VisitorT& visitor) const
    {
        BOOST_ASSERT(m_type_idx < static_cast< unsigned int >(mpl::size< value_type >::value));
        return apply_visitor_dispatch< value_type, VisitorT >::call(m_ptr, m_type_idx, visitor);
    }
};

template< typename T, typename TagT >
struct value_ref_base
{
    typedef typename mpl::eval_if<
        mpl::and_< mpl::is_sequence< T >, mpl::equal_to< mpl::size< T >, mpl::int_< 1 > > >,
        mpl::front< T >,
        mpl::identity< T >
    >::type value_type;

    typedef typename mpl::if_<
        mpl::is_sequence< value_type >,
        variant_ref< value_type, TagT >,
        singular_ref< value_type, TagT >
    >::type type;
};

} // namespace aux

/*!
 * \brief Reference wrapper for a stored attribute value.
 *
 * The \c value_ref class template provides access to the stored attribute value. It is not a traditional reference wrapper
 * since it may be empty (i.e. refer to no value at all) and it can also refer to values of different types. Therefore its
 * interface and behavior combines features of Boost.Ref, Boost.Optional and Boost.Variant, depending on the use case.
 *
 * The template parameter \c T can be a single type or an MPL sequence of possible types being referred. The reference wrapper
 * will act as either an optional reference or an optional variant of references to the specified types. In any case, the
 * referred values will not be modifiable (i.e. \c value_ref always models a const reference).
 *
 * Template parameter \c TagT is optional. It can be used for customizing the operations on this reference wrapper, such as
 * putting the referred value to log.
 */
template< typename T, typename TagT >
class value_ref :
    public aux::value_ref_base< T, TagT >::type
{
#ifndef BOOST_LOG_DOXYGEN_PASS
public:
    typedef void _has_basic_formatting_ostream_insert_operator;
#endif

private:
    //! Base implementation type
    typedef typename aux::value_ref_base< T, TagT >::type base_type;
    //! Traits for testing type compatibility with the reference wrapper
    typedef typename base_type::compatibility_traits compatibility_traits;

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Referenced value type
    typedef typename base_type::value_type value_type;
#else
    //! Referenced value type
    typedef T value_type;
    //! Tag type
    typedef TagT tag_type;
#endif

public:
    /*!
     * Default constructor. Creates a reference wrapper that does not refer to a value.
     */
    BOOST_DEFAULTED_FUNCTION(value_ref(), BOOST_NOEXCEPT {})

    /*!
     * Copy constructor.
     */
    BOOST_DEFAULTED_FUNCTION(value_ref(value_ref const& that), BOOST_NOEXCEPT : base_type(static_cast< base_type const& >(that)) {})

    /*!
     * Initializing constructor. Creates a reference wrapper that refers to the specified value.
     */
    template< typename U >
    explicit value_ref(U const& val
#ifndef BOOST_LOG_DOXYGEN_PASS
// MSVC-8 can't handle SFINAE in this case properly and often wrongly disables this constructor
#if !defined(_MSC_VER) || (_MSC_VER + 0) >= 1500
        , typename boost::enable_if_c< compatibility_traits::BOOST_NESTED_TEMPLATE is_compatible< value_type, U >::value, boost::log::aux::sfinae_dummy >::type = boost::log::aux::sfinae_dummy()
#endif
#endif
    ) BOOST_NOEXCEPT :
        base_type(boost::addressof(val))
    {
    }

    /*!
     * The operator verifies if the wrapper refers to a value.
     */
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()

    /*!
     * The operator verifies if the wrapper does not refer to a value.
     */
    bool operator! () const BOOST_NOEXCEPT
    {
        return !this->m_ptr;
    }

    /*!
     * \return \c true if the wrapper does not refer to a value.
     */
    bool empty() const BOOST_NOEXCEPT
    {
        return !this->m_ptr;
    }

    /*!
     * Swaps two reference wrappers
     */
    void swap(value_ref& that) BOOST_NOEXCEPT
    {
        base_type::swap(that);
    }
};

//! Free swap function
template< typename T, typename TagT >
inline void swap(value_ref< T, TagT >& left, value_ref< T, TagT >& right)
{
    left.swap(right);
}

//! Stream output operator
template< typename CharT, typename TraitsT, typename T, typename TagT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, value_ref< T, TagT > const& val)
{
    if (!!val)
        val.apply_visitor(boost::log::bind_output(strm));
    return strm;
}

//! Log formatting operator
template< typename CharT, typename TraitsT, typename AllocatorT, typename T, typename TagT >
inline basic_formatting_ostream< CharT, TraitsT, AllocatorT >& operator<< (basic_formatting_ostream< CharT, TraitsT, AllocatorT >& strm, value_ref< T, TagT > const& val)
{
    if (!!val)
        val.apply_visitor(boost::log::bind_to_log< TagT >(strm));
    return strm;
}

// Equality comparison
template< typename T, typename TagT, typename U >
inline bool operator== (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< equal_to, U const& >(equal_to(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator== (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< equal_to, U const& >(equal_to(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator== (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    if (!left && !right)
        return true;
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, equal_to >(right, false), false);
}

// Inequality comparison
template< typename T, typename TagT, typename U >
inline bool operator!= (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< not_equal_to, U const& >(not_equal_to(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator!= (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< not_equal_to, U const& >(not_equal_to(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator!= (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    if (!left && !right)
        return false;
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, not_equal_to >(right, false), false);
}

// Less than ordering
template< typename T, typename TagT, typename U >
inline bool operator< (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< less, U const& >(less(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator< (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< less, U const& >(less(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator< (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, less >(right, false), false);
}

// Greater than ordering
template< typename T, typename TagT, typename U >
inline bool operator> (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< greater, U const& >(greater(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator> (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< greater, U const& >(greater(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator> (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, greater >(right, false), false);
}

// Less or equal ordering
template< typename T, typename TagT, typename U >
inline bool operator<= (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< less_equal, U const& >(less_equal(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator<= (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< less_equal, U const& >(less_equal(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator<= (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    if (!left && !right)
        return true;
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, less_equal >(right, false), false);
}

// Greater or equal ordering
template< typename T, typename TagT, typename U >
inline bool operator>= (value_ref< T, TagT > const& left, U const& right)
{
    return left.apply_visitor_or_default(binder2nd< greater_equal, U const& >(greater_equal(), right), false);
}

template< typename U, typename T, typename TagT >
inline bool operator>= (U const& left, value_ref< T, TagT > const& right)
{
    return right.apply_visitor_or_default(binder1st< greater_equal, U const& >(greater_equal(), left), false);
}

template< typename T1, typename TagT1, typename T2, typename TagT2 >
inline bool operator>= (value_ref< T1, TagT1 > const& left, value_ref< T2, TagT2 > const& right)
{
    if (!left && !right)
        return true;
    return left.apply_visitor_or_default(aux::vistation_invoker< value_ref< T2, TagT2 >, greater_equal >(right, false), false);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_VALUE_REF_HPP_INCLUDED_
