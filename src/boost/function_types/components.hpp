
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#ifndef BOOST_FT_COMPONENTS_HPP_INCLUDED
#define BOOST_FT_COMPONENTS_HPP_INCLUDED

#include <cstddef>

#include <boost/config.hpp>

#include <boost/detail/workaround.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>

#include <boost/type_traits/integral_constant.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/vector/vector0.hpp>

#if BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x565)
#   include <boost/type_traits/remove_cv.hpp>

#   include <boost/mpl/identity.hpp>
#   include <boost/mpl/bitand.hpp>
#   include <boost/mpl/vector/vector10.hpp>
#   include <boost/mpl/front.hpp>
#   include <boost/mpl/begin.hpp>
#   include <boost/mpl/advance.hpp>
#   include <boost/mpl/iterator_range.hpp>
#   include <boost/mpl/joint_view.hpp>
#   include <boost/mpl/equal_to.hpp>
#   include <boost/mpl/copy.hpp>
#   include <boost/mpl/front_inserter.hpp>

#   include <boost/function_types/detail/classifier.hpp>
#endif

#ifndef BOOST_FT_NO_CV_FUNC_SUPPORT
#   include <boost/mpl/remove.hpp>
#endif

#include <boost/function_types/config/config.hpp>

#   if   BOOST_FT_MAX_ARITY < 10
#     include <boost/mpl/vector/vector10.hpp>
#   elif BOOST_FT_MAX_ARITY < 20
#     include <boost/mpl/vector/vector20.hpp>
#   elif BOOST_FT_MAX_ARITY < 30
#     include <boost/mpl/vector/vector30.hpp>
#   elif BOOST_FT_MAX_ARITY < 40
#     include <boost/mpl/vector/vector40.hpp>
#   elif BOOST_FT_MAX_ARITY < 50
#     include <boost/mpl/vector/vector50.hpp>
#   endif

#include <boost/function_types/detail/class_transform.hpp>
#include <boost/function_types/property_tags.hpp>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace boost 
{ 
  namespace function_types 
  {

    using mpl::placeholders::_;

    template< typename T, typename ClassTypeTransform = add_reference<_> > 
    struct components;

    namespace detail 
    {
      template<typename T, typename L> struct components_impl;
#if BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x565)
      template<typename T, typename OrigT, typename L> struct components_bcc;
#endif
    }

    template<typename T, typename ClassTypeTransform> 
    struct components
#if !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x565)
      : detail::components_impl<T, ClassTypeTransform>
#else
      : detail::components_bcc<typename remove_cv<T>::type,T,
            ClassTypeTransform>
#endif
    { 
      typedef components<T,ClassTypeTransform> type;

      BOOST_MPL_AUX_LAMBDA_SUPPORT(2,components,(T,ClassTypeTransform))
    };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  namespace detail {

    struct components_mpl_sequence_tag; 

    struct components_non_func_base
    {
      typedef mpl::vector0<> types;
      typedef void function_arity;

      typedef detail::constant<0> bits;
      typedef detail::constant<0> mask;

      typedef components_mpl_sequence_tag tag;
    };

    template
    < typename Components
    , typename IfTagged
    , typename ThenTag
    , typename DefaultBase = components_non_func_base
    >
    struct retagged_if
      : mpl::if_
        < detail::represents_impl<Components, IfTagged>
        , detail::changed_tag<Components,IfTagged,ThenTag>
        , DefaultBase
        >::type
    { };

    // We detect plain function types and function references as function 
    // pointers by recursive instantiation of components_impl. 
    // The third specialization of components_impl makes sure the recursion 
    // terminates (when adding pointers).
    template<typename T, typename L>
    struct components_impl
      : detail::retagged_if
        < detail::components_impl<T*,L>
        , pointer_tag, /* --> */ function_tag >
    { };
    template<typename T, typename L>
    struct components_impl<T&, L>
      : detail::retagged_if
        < detail::components_impl<T*,L>
        , pointer_tag, /* --> */ reference_tag >
    { };

#if !BOOST_FT_NO_CV_FUNC_SUPPORT
    // Retry the type with a member pointer attached to detect cv functions
    class a_class;

    template<typename Base, typename T, typename L>
    struct cv_func_base
      : detail::retagged_if<Base,member_pointer_tag,function_tag>
    {
      typedef typename
        mpl::remove
          < typename Base::types
          , typename detail::class_transform<a_class,L>::type>::type
      types;
    };

    template<typename T, typename L>
    struct components_impl<T*, L>
      : mpl::if_
        < detail::represents_impl< detail::components_impl<T a_class::*, L>
                                 , member_pointer_tag >
        , detail::cv_func_base< detail::components_impl<T a_class::*, L>, T, L>
        , components_non_func_base
        >::type
    { };

    template<typename T, typename L>
    struct components_impl<T a_class::*, L>
      : components_non_func_base
    { };
#else
    template<typename T, typename L>
    struct components_impl<T*, L>
      : components_non_func_base
    { }; 
#endif

    template<typename T, typename L>
    struct components_impl<T* const, L> 
      : components_impl<T*,L>
    { };

    template<typename T, typename L>
    struct components_impl<T* volatile, L> 
      : components_impl<T*,L>
    { };

    template<typename T, typename L>
    struct components_impl<T* const volatile, L> 
      : components_impl<T*,L>
    { };

    template<typename T, typename L>
    struct components_impl<T const, L> 
      : components_impl<T,L>
    { };

    template<typename T, typename L>
    struct components_impl<T volatile, L> 
      : components_impl<T,L>
    { };

    template<typename T, typename L>
    struct components_impl<T const volatile, L> 
      : components_impl<T,L>
    { };


    template<typename T, class C>
    struct member_obj_ptr_result
    { typedef T & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T, C const>
    { typedef T const & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T, C volatile>
    { typedef T volatile & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T, C const volatile>
    { typedef T const volatile & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T &, C>
    { typedef T & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T &, C const>
    { typedef T & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T &, C volatile>
    { typedef T & type; };

    template<typename T, class C>
    struct member_obj_ptr_result<T &, C const volatile>
    { typedef T & type; };

    template<typename T, class C, typename L>
    struct member_obj_ptr_components
      : member_object_pointer_base
    {
      typedef function_types::components<T C::*, L> type;
      typedef components_mpl_sequence_tag tag;

      typedef mpl::integral_c<std::size_t,1> function_arity;

      typedef mpl::vector2< typename detail::member_obj_ptr_result<T,C>::type,
          typename detail::class_transform<C,L>::type > types;
    };

#if !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x565)
#   define BOOST_FT_variations BOOST_FT_pointer|BOOST_FT_member_pointer

    template<typename T, class C, typename L>
    struct components_impl<T C::*, L>
      : member_obj_ptr_components<T,C,L>
    { };

#else  
#   define BOOST_FT_variations BOOST_FT_pointer

    // This workaround removes the member pointer from the type to allow 
    // detection of member function pointers with BCC. 
    template<typename T, typename C, typename L>
    struct components_impl<T C::*, L>
      : detail::retagged_if
        < detail::components_impl<typename boost::remove_cv<T>::type *, L>
        , pointer_tag, /* --> */ member_function_pointer_tag
        , member_obj_ptr_components<T,C,L> >
    { };

    // BCC lets us test the cv-qualification of a function type by template 
    // partial specialization - so we use this bug feature to find out the 
    // member function's cv-qualification (unfortunately there are some 
    // invisible modifiers that impose some limitations on these types even if
    // we remove the qualifiers, So we cannot exploit the same bug to make the 
    // library work for cv-qualified function types).
    template<typename T> struct encode_cv
    { typedef char (& type)[1]; BOOST_STATIC_CONSTANT(std::size_t, value = 1); };
    template<typename T> struct encode_cv<T const *>
    { typedef char (& type)[2]; BOOST_STATIC_CONSTANT(std::size_t, value = 2); };
    template<typename T> struct encode_cv<T volatile *>
    { typedef char (& type)[3]; BOOST_STATIC_CONSTANT(std::size_t, value = 3); };
    template<typename T> struct encode_cv<T const volatile *> 
    { typedef char (& type)[4]; BOOST_STATIC_CONSTANT(std::size_t, value = 4); };

    // For member function pointers we have to use a function template (partial
    // template specialization for a member pointer drops the cv qualification 
    // of the function type).
    template<typename T, typename C>
    typename encode_cv<T *>::type mfp_cv_tester(T C::*);

    template<typename T> struct encode_mfp_cv
    { 
      BOOST_STATIC_CONSTANT(std::size_t, value = 
          sizeof(detail::mfp_cv_tester((T)0L))); 
    };

    // Associate bits with the CV codes above.
    template<std::size_t> struct cv_tag_mfp_impl;

    template<typename T> struct cv_tag_mfp
      : detail::cv_tag_mfp_impl
        < ::boost::function_types::detail::encode_mfp_cv<T>::value >
    { };

    template<> struct cv_tag_mfp_impl<1> : non_cv              { };
    template<> struct cv_tag_mfp_impl<2> : const_non_volatile  { };
    template<> struct cv_tag_mfp_impl<3> : volatile_non_const  { };
    template<> struct cv_tag_mfp_impl<4> : cv_qualified        { };

    // Metafunction to decode the cv code and apply it to a type.
    // We add a pointer, because otherwise cv-qualifiers won't stick (another bug).
    template<typename T, std::size_t CV> struct decode_cv;

    template<typename T> struct decode_cv<T,1> : mpl::identity<T *>          {};
    template<typename T> struct decode_cv<T,2> : mpl::identity<T const *>    {};
    template<typename T> struct decode_cv<T,3> : mpl::identity<T volatile *> {};
    template<typename T> struct decode_cv<T,4> 
                                         : mpl::identity<T const volatile *> {};

    // The class type transformation comes after adding cv-qualifiers. We have
    // wrap it to remove the pointer added in decode_cv_impl.
    template<typename T, typename L> struct bcc_class_transform_impl;
    template<typename T, typename L> struct bcc_class_transform_impl<T *, L>
      : class_transform<T,L> 
    { };

    template<typename T, typename D, typename L> struct bcc_class_transform 
      : bcc_class_transform_impl
        < typename decode_cv
          < T
          , ::boost::function_types::detail::encode_mfp_cv<D>::value 
          >::type
        , L
        > 
    { };

    // After extracting the member pointee from the type the class type is still
    // in the type (somewhere -- you won't see with RTTI, that is) and that type
    // is flagged unusable and *not* identical to the nonmember function type.
    // We can, however, decompose this type via components_impl but surprisingly
    // a pointer to the const qualified class type pops up again as the first 
    // parameter type. 
    // We have to replace this type with the properly cv-qualified and 
    // transformed class type, integrate the cv qualification into the bits.
    template<typename Base, typename MFP, typename OrigT, typename L>
    struct mfp_components;


    template<typename Base, typename T, typename C, typename OrigT, typename L>
    struct mfp_components<Base,T C::*,OrigT,L> 
    {
    private:
      typedef typename mpl::front<typename Base::types>::type result_type;
      typedef typename detail::bcc_class_transform<C,OrigT,L>::type class_type;

      typedef mpl::vector2<result_type, class_type> result_and_class_type;

      typedef typename 
        mpl::advance
        < typename mpl::begin<typename Base::types>::type
        , typename mpl::if_
          < mpl::equal_to< typename detail::classifier<OrigT>::function_arity
                         , typename Base::function_arity >
          , mpl::integral_c<int,2> , mpl::integral_c<int,1> 
          >::type
        >::type
      from;
      typedef typename mpl::end<typename Base::types>::type to;

      typedef mpl::iterator_range<from,to> param_types;

      typedef mpl::joint_view< result_and_class_type, param_types> types_view;
    public:

      typedef typename 
        mpl::reverse_copy<types_view, mpl::front_inserter< mpl::vector0<> > >::type 
      types;

      typedef typename 
        function_types::tag< Base, detail::cv_tag_mfp<OrigT> >::bits 
      bits;

      typedef typename Base::mask mask;

      typedef typename detail::classifier<OrigT>::function_arity function_arity;

      typedef components_mpl_sequence_tag tag;
    };

    // Now put it all together: detect cv-qualification of function types and do
    // the weird transformations above for member function pointers.
    template<typename T, typename OrigT, typename L>
    struct components_bcc
      : mpl::if_
        < detail::represents_impl< detail::components_impl<T,L>
                                 , member_function_pointer_tag>
        , detail::mfp_components<detail::components_impl<T,L>,T,OrigT,L>
        , detail::components_impl<T,L>
        >::type
    { };

#endif // end of BORLAND WORKAROUND

#define BOOST_FT_al_path boost/function_types/detail/components_impl
#include <boost/function_types/detail/pp_loop.hpp>

  } } // namespace function_types::detail

} // namespace ::boost

#include <boost/function_types/detail/components_as_mpl_sequence.hpp>
#include <boost/function_types/detail/retag_default_cc.hpp>

#endif

