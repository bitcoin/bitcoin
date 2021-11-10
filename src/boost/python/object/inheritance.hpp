// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef INHERITANCE_DWA200216_HPP
# define INHERITANCE_DWA200216_HPP

# include <boost/python/type_id.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/mpl/if.hpp>
# include <boost/detail/workaround.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python { namespace objects {

typedef type_info class_id;
using python::type_id;

// Types used to get address and id of most derived type
typedef std::pair<void*,class_id> dynamic_id_t;
typedef dynamic_id_t (*dynamic_id_function)(void*);

BOOST_PYTHON_DECL void register_dynamic_id_aux(
    class_id static_id, dynamic_id_function get_dynamic_id);

BOOST_PYTHON_DECL void add_cast(
    class_id src_t, class_id dst_t, void* (*cast)(void*), bool is_downcast);

//
// a generator with an execute() function which, given a source type
// and a pointer to an object of that type, returns its most-derived
// /reachable/ type identifier and object pointer.
//

// first, the case where T has virtual functions
template <class T>
struct polymorphic_id_generator
{
    static dynamic_id_t execute(void* p_)
    {
        T* p = static_cast<T*>(p_);
        return std::make_pair(dynamic_cast<void*>(p), class_id(typeid(*p)));
    }
};

// now, the non-polymorphic case.
template <class T>
struct non_polymorphic_id_generator
{
    static dynamic_id_t execute(void* p_)
    {
        return std::make_pair(p_, python::type_id<T>());
    }
};

// Now the generalized selector
template <class T>
struct dynamic_id_generator
  : mpl::if_<
        boost::python::detail::is_polymorphic<T>
        , boost::python::objects::polymorphic_id_generator<T>
        , boost::python::objects::non_polymorphic_id_generator<T>
    >
{};

// Register the dynamic id function for T with the type-conversion
// system.
template <class T>
void register_dynamic_id(T* = 0)
{
    typedef typename dynamic_id_generator<T>::type generator;
    register_dynamic_id_aux(
        python::type_id<T>(), &generator::execute);
}

//
// a generator with an execute() function which, given a void*
// pointing to an object of type Source will attempt to convert it to
// an object of type Target.
//

template <class Source, class Target>
struct dynamic_cast_generator
{
    static void* execute(void* source)
    {
        return dynamic_cast<Target*>(
            static_cast<Source*>(source));
    }
        
};

template <class Source, class Target>
struct implicit_cast_generator
{
    static void* execute(void* source)
    {
        Target* result = static_cast<Source*>(source);
        return result;
    }
};

template <class Source, class Target>
struct cast_generator
  : mpl::if_<
        boost::python::detail::is_base_and_derived<Target,Source>
      , implicit_cast_generator<Source,Target>
      , dynamic_cast_generator<Source,Target>
    >
{
};

template <class Source, class Target>
inline void register_conversion(
    bool is_downcast = ::boost::is_base_and_derived<Source,Target>::value
    // These parameters shouldn't be used; they're an MSVC bug workaround
    , Source* = 0, Target* = 0)
{
    typedef typename cast_generator<Source,Target>::type generator;

    add_cast(
        python::type_id<Source>()
      , python::type_id<Target>()
      , &generator::execute
      , is_downcast
    );
}

}}} // namespace boost::python::object

#endif // INHERITANCE_DWA200216_HPP
