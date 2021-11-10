#ifndef BOOST_RANGE_ATL_HPP
#define BOOST_RANGE_ATL_HPP




// Boost.Range ATL Extension
//
// Copyright Shunsuke Sogame 2005-2006.
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)




// config
//


#include <atldef.h> // _ATL_VER


#if !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)
    #if (_ATL_VER < 0x0700)
        #define BOOST_RANGE_ATL_NO_COLLECTIONS
    #endif
#endif


#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)
    #if (_ATL_VER < 0x0700) // dubious
        #define BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX
    #endif
#endif


// forward declarations
//


#include <basetyps.h> // IID


namespace ATL {


#if !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


    // arrays
    //
    template< class E, class ETraits >
    class CAtlArray;

    template< class E >
    class CAutoPtrArray;

    template< class I, const IID *piid >
    class CInterfaceArray;


    // lists
    //
    template< class E, class ETraits >
    class CAtlList;

    template< class E >
    class CAutoPtrList;

    template< class E, class Allocator >
    class CHeapPtrList;

    template< class I, const IID *piid >
    class CInterfaceList;


    // maps
    //
    template< class K, class V, class KTraits, class VTraits >
    class CAtlMap;

    template< class K, class V, class KTraits, class VTraits >
    class CRBTree;

    template< class K, class V, class KTraits, class VTraits >
    class CRBMap;

    template< class K, class V, class KTraits, class VTraits >
    class CRBMultiMap;


    // strings
    //
#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLESTRING)
    template< class BaseType, bool t_bMFCDLL >
    class CSimpleStringT;
#else
    template< class BaseType >
    class CSimpleStringT;
#endif

    template< class BaseType, class StringTraits >
    class CStringT;

    template< class StringType, int t_nChars >
    class CFixedStringT;

    template< class BaseType, const int t_nSize >
    class CStaticString;


#endif // !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


    // simples
    //
#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)

    template< class T, class TEqual >
    class CSimpleArray;

    template< class TKey, class TVal, class TEqual >
    class CSimpleMap;

#else

    template< class T >
    class CSimpleArray;

    template< class T >
    class CSimpleValArray;

    template< class TKey, class TVal >
    class CSimpleMap;

#endif // !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)


    // pointers
    //
    template< class E >
    class CAutoPtr;

    template< class T >
    class CComPtr;

    template< class T, const IID *piid >
    class CComQIPtr;

    template< class E, class Allocator >
    class CHeapPtr;

    template< class T >
    class CAdapt;


} // namespace ATL




// indirect_iterator customizations
//


#include <boost/mpl/identity.hpp>
#include <boost/pointee.hpp>


namespace boost {


    template< class E >
    struct pointee< ATL::CAutoPtr<E> > :
        mpl::identity<E>
    { };

    template< class T >
    struct pointee< ATL::CComPtr<T> > :
        mpl::identity<T>
    { };

    template< class T, const IID *piid >
    struct pointee< ATL::CComQIPtr<T, piid> > :
        mpl::identity<T>
    { };

    template< class E, class Allocator >
    struct pointee< ATL::CHeapPtr<E, Allocator> > :
        mpl::identity<E>
    { };

    template< class T >
    struct pointee< ATL::CAdapt<T> > :
        pointee<T>
    { };


} // namespace boost




// extended customizations
//


#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/detail/microsoft.hpp>
#include <boost/tuple/tuple.hpp>
#include <atlbase.h> // CComBSTR


namespace boost { namespace range_detail_microsoft {


#if !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


    // arrays
    //

    struct atl_array_functions :
        array_functions
    {
        template< class Iterator, class X >
        Iterator end(X& x) // redefine
        {
            return x.GetData() + x.GetCount(); // no 'GetSize()'
        }
    };


    template< class E, class ETraits >
    struct customization< ATL::CAtlArray<E, ETraits> > :
        atl_array_functions
    {
        template< class X >
        struct meta
        {
            typedef E val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< class E >
    struct customization< ATL::CAutoPtrArray<E> > :
        atl_array_functions
    {
        template< class X >
        struct meta
        {
            // ATL::CAutoPtr/CHeapPtr is no assignable.
            typedef ATL::CAutoPtr<E> val_t;
            typedef val_t *miter_t;
            typedef val_t const *citer_t;

            typedef indirect_iterator<miter_t> mutable_iterator;
            typedef indirect_iterator<citer_t> const_iterator;
        };
    };


    template< class I, const IID *piid >
    struct customization< ATL::CInterfaceArray<I, piid> > :
        atl_array_functions
    {
        template< class X >
        struct meta
        {
            typedef ATL::CComQIPtr<I, piid> val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< class E, class ETraits >
    struct customization< ATL::CAtlList<E, ETraits> > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef E val_t;

            typedef list_iterator<X, val_t> mutable_iterator;
            typedef list_iterator<X const, val_t const> const_iterator;
        };
    };


    struct indirected_list_functions
    {
        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            typedef typename Iterator::base_type base_t; // == list_iterator
            return Iterator(base_t(x, x.GetHeadPosition()));
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            typedef typename Iterator::base_type base_t;
            return Iterator(base_t(x, POSITION(0)));
        }
    };


    template< class E >
    struct customization< ATL::CAutoPtrList<E> > :
        indirected_list_functions
    {
        template< class X >
        struct meta
        {
            typedef ATL::CAutoPtr<E> val_t;
            typedef list_iterator<X, val_t> miter_t;
            typedef list_iterator<X const, val_t const> citer_t;

            typedef indirect_iterator<miter_t> mutable_iterator;
            typedef indirect_iterator<citer_t> const_iterator;
        };
    };


    template< class E, class Allocator >
    struct customization< ATL::CHeapPtrList<E, Allocator> > :
        indirected_list_functions
    {
        template< class X >
        struct meta
        {
            typedef ATL::CHeapPtr<E, Allocator> val_t;
            typedef list_iterator<X, val_t> miter_t;
            typedef list_iterator<X const, val_t const> citer_t;

            typedef indirect_iterator<miter_t> mutable_iterator;
            typedef indirect_iterator<citer_t> const_iterator;
        };
    };


    template< class I, const IID *piid >
    struct customization< ATL::CInterfaceList<I, piid> > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef ATL::CComQIPtr<I, piid> val_t;

            typedef list_iterator<X, val_t> mutable_iterator;
            typedef list_iterator<X const, val_t const> const_iterator;
        };
    };


    // maps
    //

    struct atl_rb_tree_tag
    { };

    template< >
    struct customization< atl_rb_tree_tag > :
        indirected_list_functions
    {
        template< class X >
        struct meta
        {
            typedef typename X::CPair val_t;

            typedef list_iterator<X, val_t *, val_t *> miter_t;
            typedef list_iterator<X const, val_t const *, val_t const *> citer_t;
            
            typedef indirect_iterator<miter_t> mutable_iterator;
            typedef indirect_iterator<citer_t> const_iterator;
        };
    };


    template< class K, class V, class KTraits, class VTraits >
    struct customization< ATL::CAtlMap<K, V, KTraits, VTraits> > :
        customization< atl_rb_tree_tag >
    {
        template< class Iterator, class X >
        Iterator begin(X& x) // redefine
        {
            typedef typename Iterator::base_type base_t; // == list_iterator
            return Iterator(base_t(x, x.GetStartPosition())); // no 'GetHeadPosition'
        }
    };


    // strings
    //

    struct atl_string_tag
    { };

    template< >
    struct customization< atl_string_tag >
    {
        template< class X >
        struct meta
        {
            typedef typename X::PXSTR mutable_iterator;
            typedef typename X::PCXSTR const_iterator;
        };

        template< class Iterator, class X >
        typename mutable_<Iterator, X>::type begin(X& x)
        {
            return x.GetBuffer(0);
        }

        template< class Iterator, class X >
        Iterator begin(X const& x)
        {
            return x.GetString();
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return begin<Iterator>(x) + x.GetLength();
        }
    };


    template< class BaseType, const int t_nSize >
    struct customization< ATL::CStaticString<BaseType, t_nSize> >
    {
        template< class X >
        struct meta
        {
            typedef BaseType const *mutable_iterator;
            typedef mutable_iterator const_iterator;
        };

        template< class Iterator, class X >
        Iterator begin(X const& x)
        {
            return x;
        }

        template< class Iterator, class X >
        Iterator end(X const& x)
        {
            return begin<Iterator>(x) + X::GetLength();
        }
    };


#endif // !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


    template< >
    struct customization< ATL::CComBSTR >
    {
        template< class X >
        struct meta
        {
            typedef OLECHAR *mutable_iterator;
            typedef OLECHAR const *const_iterator;
        };

        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            return x.operator BSTR();
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return begin<Iterator>(x) + x.Length();
        }
    };


    // simples
    //

#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)
    template< class T, class TEqual >
    struct customization< ATL::CSimpleArray<T, TEqual> > :
#else
    template< class T >
    struct customization< ATL::CSimpleArray<T> > :
#endif
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef T val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


#if defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)

    template< class T >
    struct customization< ATL::CSimpleValArray<T> > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef T val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };

#endif // defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)


#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)
    template< class TKey, class TVal, class TEqual >
    struct customization< ATL::CSimpleMap<TKey, TVal, TEqual> >
#else
    template< class TKey, class TVal >
    struct customization< ATL::CSimpleMap<TKey, TVal> >
#endif
    {
        template< class X >
        struct meta
        {
            typedef TKey k_val_t;
            typedef k_val_t *k_miter_t;
            typedef k_val_t const *k_citer_t;

            typedef TVal v_val_t;
            typedef v_val_t *v_miter_t;
            typedef v_val_t const *v_citer_t;

            // Topic:
            // 'std::pair' can't contain references
            // because of reference to reference problem.

            typedef zip_iterator< tuple<k_miter_t, v_miter_t> > mutable_iterator;
            typedef zip_iterator< tuple<k_citer_t, v_citer_t> > const_iterator;
        };

        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            return Iterator(boost::make_tuple(x.m_aKey, x.m_aVal));
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return Iterator(boost::make_tuple(x.m_aKey + x.GetSize(), x.m_aVal + x.GetSize()));
        }
    };


} } // namespace boost::range_detail_microsoft




// range customizations
//


#if !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


    // arrays
    //
    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CAtlArray, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CAutoPtrArray, 1
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CInterfaceArray, (class)(const IID *)
    )


    // lists
    //
    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CAtlList, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CAutoPtrList, 1
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CHeapPtrList, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CInterfaceList, (class)(const IID *)
    )


    //maps
    //
    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CAtlMap, 4
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::atl_rb_tree_tag,
        (ATL, BOOST_PP_NIL), CRBTree, 4
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::atl_rb_tree_tag,
        (ATL, BOOST_PP_NIL), CRBMap, 4
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::atl_rb_tree_tag,
        (ATL, BOOST_PP_NIL), CRBMultiMap, 4
    )


    // strings
    //
    #if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLESTRING)
        BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
            boost::range_detail_microsoft::atl_string_tag,
            (ATL, BOOST_PP_NIL), CSimpleStringT, (class)(bool)
        )
    #else
        BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
            boost::range_detail_microsoft::atl_string_tag,
            (ATL, BOOST_PP_NIL), CSimpleStringT, 1
        )
    #endif

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::atl_string_tag,
        (ATL, BOOST_PP_NIL), CStringT, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::atl_string_tag,
        (ATL, BOOST_PP_NIL), CFixedStringT, (class)(int)
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CStaticString, (class)(const int)
    )


#endif // !defined(BOOST_RANGE_ATL_NO_COLLECTIONS)


BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    (ATL, BOOST_PP_NIL), CComBSTR
)


// simples
//
#if !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CSimpleArray, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CSimpleMap, 3
    )

#else

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CSimpleArray, 1
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CSimpleMap, 2
    )

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
        boost::range_detail_microsoft::using_type_as_tag,
        (ATL, BOOST_PP_NIL), CSimpleValArray, 1
    )

#endif // !defined(BOOST_RANGE_ATL_HAS_OLD_CSIMPLE_XXX)




#endif
