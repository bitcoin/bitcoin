#ifndef BOOST_SERIALIZATION_EXPORT_HPP
#define BOOST_SERIALIZATION_EXPORT_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// export.hpp: set traits of classes to be serialized

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// (C) Copyright 2006 David Abrahams - http://www.boost.org.
// implementation of class export functionality.  This is an alternative to
// "forward declaration" method to provoke instantiation of derived classes
// that are to be serialized through pointers.

#include <utility>
#include <cstddef> // NULL

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/type_traits/is_polymorphic.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/bool_fwd.hpp>

#include <boost/serialization/extended_type_info.hpp> // for guid_defined only
#include <boost/serialization/static_warning.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/force_include.hpp>
#include <boost/serialization/singleton.hpp>

#include <boost/archive/detail/register_archive.hpp>

namespace boost {
namespace archive {
namespace detail {

class basic_pointer_iserializer;
class basic_pointer_oserializer;

template<class Archive, class T>
class pointer_iserializer;
template<class Archive, class T>
class pointer_oserializer;

template <class Archive, class Serializable>
struct export_impl
{
    static const basic_pointer_iserializer &
    enable_load(mpl::true_){
        return boost::serialization::singleton<
            pointer_iserializer<Archive, Serializable>
        >::get_const_instance();
    }

    static const basic_pointer_oserializer &
    enable_save(mpl::true_){
        return boost::serialization::singleton<
            pointer_oserializer<Archive, Serializable>
        >::get_const_instance();
    }
    inline static void enable_load(mpl::false_) {}
    inline static void enable_save(mpl::false_) {}
};

// On many platforms, naming a specialization of this template is
// enough to cause its argument to be instantiated.
template <void(*)()>
struct instantiate_function {};

template <class Archive, class Serializable>
struct ptr_serialization_support
{
# if defined(BOOST_MSVC) || defined(__SUNPRO_CC)
    virtual BOOST_DLLEXPORT void instantiate() BOOST_USED;
# else
    static BOOST_DLLEXPORT void instantiate() BOOST_USED;
    typedef instantiate_function<
        &ptr_serialization_support::instantiate
    > x;
# endif
};

template <class Archive, class Serializable>
BOOST_DLLEXPORT void
ptr_serialization_support<Archive,Serializable>::instantiate()
{
    export_impl<Archive,Serializable>::enable_save(
        typename Archive::is_saving()
    );

    export_impl<Archive,Serializable>::enable_load(
        typename Archive::is_loading()
    );
}

// Note INTENTIONAL usage of anonymous namespace in header.
// This was made this way so that export.hpp could be included
// in other headers.  This is still under study.

namespace extra_detail {

template<class T>
struct guid_initializer
{
    void export_guid(mpl::false_) const {
        // generates the statically-initialized objects whose constructors
        // register the information allowing serialization of T objects
        // through pointers to their base classes.
        instantiate_ptr_serialization((T*)0, 0, adl_tag());
    }
    void export_guid(mpl::true_) const {
    }
    guid_initializer const & export_guid() const {
        BOOST_STATIC_WARNING(boost::is_polymorphic< T >::value);
        // note: exporting an abstract base class will have no effect
        // and cannot be used to instantitiate serialization code
        // (one might be using this in a DLL to instantiate code)
        //BOOST_STATIC_WARNING(! boost::serialization::is_abstract< T >::value);
        export_guid(boost::serialization::is_abstract< T >());
        return *this;
    }
};

template<typename T>
struct init_guid;

} // anonymous
} // namespace detail
} // namespace archive
} // namespace boost

#define BOOST_CLASS_EXPORT_IMPLEMENT(T)                      \
    namespace boost {                                        \
    namespace archive {                                      \
    namespace detail {                                       \
    namespace extra_detail {                                 \
    template<>                                               \
    struct init_guid< T > {                                  \
        static guid_initializer< T > const & g;              \
    };                                                       \
    guid_initializer< T > const & init_guid< T >::g =        \
        ::boost::serialization::singleton<                   \
            guid_initializer< T >                            \
        >::get_mutable_instance().export_guid();             \
    }}}}                                                     \
/**/

#define BOOST_CLASS_EXPORT_KEY2(T, K)          \
namespace boost {                              \
namespace serialization {                      \
template<>                                     \
struct guid_defined< T > : boost::mpl::true_ {}; \
template<>                                     \
inline const char * guid< T >(){                 \
    return K;                                  \
}                                              \
} /* serialization */                          \
} /* boost */                                  \
/**/

#define BOOST_CLASS_EXPORT_KEY(T)                                      \
    BOOST_CLASS_EXPORT_KEY2(T, BOOST_PP_STRINGIZE(T))                                                                  \
/**/

#define BOOST_CLASS_EXPORT_GUID(T, K)                                  \
BOOST_CLASS_EXPORT_KEY2(T, K)                                          \
BOOST_CLASS_EXPORT_IMPLEMENT(T)                                        \
/**/

#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))

// CodeWarrior fails to construct static members of class templates
// when they are instantiated from within templates, so on that
// compiler we ask users to specifically register base/derived class
// relationships for exported classes.  On all other compilers, use of
// this macro is entirely optional.
# define BOOST_SERIALIZATION_MWERKS_BASE_AND_DERIVED(Base,Derived)             \
namespace {                                                                    \
  static int BOOST_PP_CAT(boost_serialization_mwerks_init_, __LINE__) =        \
  (::boost::archive::detail::instantiate_ptr_serialization((Derived*)0,0), 3); \
  static int BOOST_PP_CAT(boost_serialization_mwerks_init2_, __LINE__) = (     \
      ::boost::serialization::void_cast_register((Derived*)0,(Base*)0)         \
    , 3);                                                                      \
}

#else

# define BOOST_SERIALIZATION_MWERKS_BASE_AND_DERIVED(Base,Derived)

#endif

// check for unnecessary export.  T isn't polymorphic so there is no
// need to export it.
#define BOOST_CLASS_EXPORT_CHECK(T)                              \
    BOOST_STATIC_WARNING(                                        \
        boost::is_polymorphic<U>::value                          \
    );                                                           \
    /**/

// the default exportable class identifier is the class name
// the default list of archives types for which code id generated
// are the originally included with this serialization system
#define BOOST_CLASS_EXPORT(T)                   \
    BOOST_CLASS_EXPORT_GUID(                    \
        T,                                      \
        BOOST_PP_STRINGIZE(T)                   \
    )                                           \
    /**/

#endif // BOOST_SERIALIZATION_EXPORT_HPP

