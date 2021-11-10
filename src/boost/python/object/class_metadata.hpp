// Copyright David Abrahams 2004.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_python_object_class_metadata_hpp_
#define boost_python_object_class_metadata_hpp_

#include <boost/python/converter/shared_ptr_from_python.hpp>
#include <boost/python/object/inheritance.hpp>
#include <boost/python/object/class_wrapper.hpp>
#include <boost/python/object/make_instance.hpp>
#include <boost/python/object/value_holder.hpp>
#include <boost/python/object/pointer_holder.hpp>
#include <boost/python/object/make_ptr_instance.hpp>

#include <boost/python/detail/force_instantiate.hpp>
#include <boost/python/detail/not_specified.hpp>
#include <boost/python/detail/type_traits.hpp>

#include <boost/python/has_back_reference.hpp>
#include <boost/python/bases.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/single_view.hpp>

#include <boost/mpl/assert.hpp>

#include <boost/noncopyable.hpp>
#include <boost/detail/workaround.hpp>

namespace boost { namespace python { namespace objects { 

BOOST_PYTHON_DECL
void copy_class_object(type_info const& src, type_info const& dst);

//
// Support for registering base/derived relationships
//
template <class Derived>
struct register_base_of
{
    template <class Base>
    inline void operator()(Base*) const
    {
        BOOST_MPL_ASSERT_NOT((boost::python::detail::is_same<Base,Derived>));
        
        // Register the Base class
        register_dynamic_id<Base>();

        // Register the up-cast
        register_conversion<Derived,Base>(false);

        // Register the down-cast, if appropriate.
        this->register_downcast((Base*)0, boost::python::detail::is_polymorphic<Base>());
    }

 private:
    static inline void register_downcast(void*, boost::python::detail::false_) {}
    
    template <class Base>
    static inline void register_downcast(Base*, boost::python::detail::true_)
    {
        register_conversion<Base, Derived>(true);
    }

};

//
// Preamble of register_class.  Also used for callback classes, which
// need some registration of their own.
//

template <class T, class Bases>
inline void register_shared_ptr_from_python_and_casts(T*, Bases)
{
  // Constructor performs registration
  python::detail::force_instantiate(converter::shared_ptr_from_python<T, boost::shared_ptr>());
#if !defined(BOOST_NO_CXX11_SMART_PTR)
  python::detail::force_instantiate(converter::shared_ptr_from_python<T, std::shared_ptr>());
#endif

  //
  // register all up/downcasts here.  We're using the alternate
  // interface to mpl::for_each to avoid an MSVC 6 bug.
  //
  register_dynamic_id<T>();
  mpl::for_each(register_base_of<T>(), (Bases*)0, (boost::python::detail::add_pointer<mpl::_>*)0);
}

//
// Helper for choosing the unnamed held_type argument
//
template <class T, class Prev>
struct select_held_type
  : mpl::if_<
        mpl::or_<
            python::detail::specifies_bases<T>
          , boost::python::detail::is_same<T,noncopyable>
        >
      , Prev
      , T
    >
{
};

template <
    class T // class being wrapped
  , class X1 // = detail::not_specified
  , class X2 // = detail::not_specified
  , class X3 // = detail::not_specified
>
struct class_metadata
{
    //
    // Calculate the unnamed template arguments
    //
    
    // held_type_arg -- not_specified, [a class derived from] T or a
    // smart pointer to [a class derived from] T.  Preserving
    // not_specified allows us to give class_<T,T> a back-reference.
    typedef typename select_held_type<
        X1
      , typename select_held_type<
            X2
          , typename select_held_type<
                X3
              , python::detail::not_specified
            >::type
        >::type
    >::type held_type_arg;

    // bases
    typedef typename python::detail::select_bases<
        X1
      , typename python::detail::select_bases<
            X2
          , typename python::detail::select_bases<
                X3
              , python::bases<>
            >::type
        >::type
    >::type bases;

    typedef mpl::or_<
        boost::python::detail::is_same<X1,noncopyable>
      , boost::python::detail::is_same<X2,noncopyable>
      , boost::python::detail::is_same<X3,noncopyable>
    > is_noncopyable;
    
    //
    // Holder computation.
    //
    
    // Compute the actual type that will be held in the Holder.
    typedef typename mpl::if_<
        boost::python::detail::is_same<held_type_arg,python::detail::not_specified>, T, held_type_arg
    >::type held_type;

    // Determine if the object will be held by value
    typedef mpl::bool_<boost::python::detail::is_convertible<held_type*,T*>::value> use_value_holder;
    
    // Compute the "wrapped type", that is, if held_type is a smart
    // pointer, we're talking about the pointee.
    typedef typename mpl::eval_if<
        use_value_holder
      , mpl::identity<held_type>
      , pointee<held_type>
    >::type wrapped;

    // Determine whether to use a "back-reference holder"
    typedef mpl::bool_<
        mpl::or_<
            has_back_reference<T>
          , boost::python::detail::is_same<held_type_arg,T>
          , is_base_and_derived<T,wrapped>
        >::value
    > use_back_reference;

    // Select the holder.
    typedef typename mpl::eval_if<
        use_back_reference
      , mpl::if_<
            use_value_holder
          , value_holder_back_reference<T, wrapped>
          , pointer_holder_back_reference<held_type,T>
        >
      , mpl::if_<
            use_value_holder
          , value_holder<T>
          , pointer_holder<held_type,wrapped>
        >
    >::type holder;
    
    inline static void register_() // Register the runtime metadata.
    {
        class_metadata::register_aux((T*)0);
    }

 private:
    template <class T2>
    inline static void register_aux(python::wrapper<T2>*) 
    {
        typedef typename mpl::not_<boost::python::detail::is_same<T2,wrapped> >::type use_callback;
        class_metadata::register_aux2((T2*)0, use_callback());
    }

    inline static void register_aux(void*) 
    {
        typedef typename is_base_and_derived<T,wrapped>::type use_callback;
        class_metadata::register_aux2((T*)0, use_callback());
    }

    template <class T2, class Callback>
    inline static void register_aux2(T2*, Callback) 
    {
	objects::register_shared_ptr_from_python_and_casts((T2*)0, bases());
        class_metadata::maybe_register_callback_class((T2*)0, Callback());

        class_metadata::maybe_register_class_to_python((T2*)0, is_noncopyable());
        
        class_metadata::maybe_register_pointer_to_python(
            (T2*)0, (use_value_holder*)0, (use_back_reference*)0);
    }


    //
    // Support for converting smart pointers to python
    //
    inline static void maybe_register_pointer_to_python(...) {}

#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
    inline static void maybe_register_pointer_to_python(void*,void*,mpl::true_*)
    {
        objects::copy_class_object(python::type_id<T>(), python::type_id<back_reference<T const &> >());
        objects::copy_class_object(python::type_id<T>(), python::type_id<back_reference<T &> >());
    }
#endif

    template <class T2>
    inline static void maybe_register_pointer_to_python(T2*, mpl::false_*, mpl::false_*)
    {
        python::detail::force_instantiate(
            objects::class_value_wrapper<
                held_type
              , make_ptr_instance<T2, pointer_holder<held_type, T2> >
            >()
        );
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<held_type>());
#endif
    }
    //
    // Support for registering to-python converters
    //
    inline static void maybe_register_class_to_python(void*, mpl::true_) {}
    

    template <class T2>
    inline static void maybe_register_class_to_python(T2*, mpl::false_)
    {
        python::detail::force_instantiate(class_cref_wrapper<T2, make_instance<T2, holder> >());
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<held_type>());
#endif
    }

    //
    // Support for registering callback classes
    //
    inline static void maybe_register_callback_class(void*, mpl::false_) {}

    template <class T2>
    inline static void maybe_register_callback_class(T2*, mpl::true_)
    {
	objects::register_shared_ptr_from_python_and_casts(
            (wrapped*)0, mpl::single_view<T2>());
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<wrapped>());
    }
};

}}} // namespace boost::python::object

#endif
