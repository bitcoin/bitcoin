#ifndef BOOST_RANGE_MFC_HPP
#define BOOST_RANGE_MFC_HPP




// Boost.Range MFC Extension
//
// Copyright Shunsuke Sogame 2005-2006.
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)




// config
//


#include <afx.h> // _MFC_VER


#if !defined(BOOST_RANGE_MFC_NO_CPAIR)
    #if (_MFC_VER < 0x0700) // dubious
        #define BOOST_RANGE_MFC_NO_CPAIR
    #endif
#endif


#if !defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)
    #if (_MFC_VER < 0x0700) // dubious
        #define BOOST_RANGE_MFC_HAS_LEGACY_STRING
    #endif
#endif


// A const collection of old MFC doesn't return const reference.
//
#if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
    #if (_MFC_VER < 0x0700) // dubious
        #define BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF
    #endif
#endif




// forward declarations
//


template< class Type, class ArgType >
class CArray;

template< class Type, class ArgType >
class CList;

template< class Key, class ArgKey, class Mapped, class ArgMapped >
class CMap;

template< class BaseClass, class PtrType >
class CTypedPtrArray;

template< class BaseClass, class PtrType >
class CTypedPtrList;

template< class BaseClass, class KeyPtrType, class MappedPtrType >
class CTypedPtrMap;




// extended customizations
//


#include <cstddef> // ptrdiff_t
#include <utility> // pair
#include <boost/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range/atl.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/range/detail/microsoft.hpp>
#include <boost/range/end.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/addressof.hpp>
#include <afx.h> // legacy CString
#include <afxcoll.h> // CXXXArray, CXXXList, CMapXXXToXXX
#include <tchar.h>


namespace boost { namespace range_detail_microsoft {


    // mfc_ptr_array_iterator
    //
    // 'void **' is not convertible to 'void const **',
    // so we define...
    //

    template< class ArrayT, class PtrType >
    struct mfc_ptr_array_iterator;

    template< class ArrayT, class PtrType >
    struct mfc_ptr_array_iterator_super
    {
        typedef iterator_adaptor<
            mfc_ptr_array_iterator<ArrayT, PtrType>,
            std::ptrdiff_t, // Base!
            PtrType,        // Value
            random_access_traversal_tag,
            use_default,
            std::ptrdiff_t  // Difference
        > type;
    };

    template< class ArrayT, class PtrType >
    struct mfc_ptr_array_iterator :
        mfc_ptr_array_iterator_super<ArrayT, PtrType>::type
    {
    private:
        typedef mfc_ptr_array_iterator self_t;
        typedef typename mfc_ptr_array_iterator_super<ArrayT, PtrType>::type super_t;
        typedef typename super_t::reference ref_t;

    public:
        explicit mfc_ptr_array_iterator()
        { }

        explicit mfc_ptr_array_iterator(ArrayT& arr, INT_PTR index) :
            super_t(index), m_parr(boost::addressof(arr))
        { }

    template< class, class > friend struct mfc_ptr_array_iterator;
        template< class ArrayT_, class PtrType_ >
        mfc_ptr_array_iterator(mfc_ptr_array_iterator<ArrayT_, PtrType_> const& other) :
            super_t(other.base()), m_parr(other.m_parr)
        { }

    private:
        ArrayT *m_parr;

    friend class iterator_core_access;
        ref_t dereference() const
        {
            BOOST_ASSERT(0 <= this->base() && this->base() < m_parr->GetSize() && "out of range");
            return *( m_parr->GetData() + this->base() );
        }

        bool equal(self_t const& other) const
        {
            BOOST_ASSERT(m_parr == other.m_parr && "iterators incompatible");
            return this->base() == other.base();
        }
    };

    struct mfc_ptr_array_functions
    {
        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            return Iterator(x, 0);
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return Iterator(x, x.GetSize());
        }
    };


    // arrays
    //

    template< >
    struct customization< ::CByteArray > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef BYTE val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< >
    struct customization< ::CDWordArray > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef DWORD val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< >
    struct customization< ::CObArray > :
        mfc_ptr_array_functions
    {
        template< class X >
        struct meta
        {
            typedef mfc_ptr_array_iterator<X, CObject *> mutable_iterator;
            typedef mfc_ptr_array_iterator<X const, CObject const *> const_iterator;
        };
    };


    template< >
    struct customization< ::CPtrArray > :
        mfc_ptr_array_functions
    {
        template< class X >
        struct meta
        {
            typedef mfc_ptr_array_iterator<X, void *> mutable_iterator;
            typedef mfc_ptr_array_iterator<X const, void const *> const_iterator;
        };
    };


    template< >
    struct customization< ::CStringArray > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef ::CString val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< >
    struct customization< ::CUIntArray > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef UINT val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< >
    struct customization< ::CWordArray > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef WORD val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    // lists
    //

    template< >
    struct customization< ::CObList > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef list_iterator<X, ::CObject *> mutable_iterator;
    #if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
            typedef list_iterator<X const, ::CObject const *> const_iterator;
    #else
            typedef list_iterator<X const, ::CObject const * const, ::CObject const * const> const_iterator;
    #endif
        };
    };


    template< >
    struct customization< ::CPtrList > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef list_iterator<X, void *> mutable_iterator;
    #if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
            typedef list_iterator<X const, void const *> const_iterator;
    #else
            typedef list_iterator<X const, void const * const, void const * const> const_iterator;
    #endif
        };
    };


    template< >
    struct customization< ::CStringList > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef ::CString val_t;

            typedef list_iterator<X, val_t> mutable_iterator;
    #if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
            typedef list_iterator<X const, val_t const> const_iterator;
    #else
            typedef list_iterator<X const, val_t const, val_t const> const_iterator;
    #endif
        };
    };


    // mfc_map_iterator
    //

    template< class MapT, class KeyT, class MappedT >
    struct mfc_map_iterator;

    template< class MapT, class KeyT, class MappedT >
    struct mfc_map_iterator_super
    {
        typedef iterator_facade<
            mfc_map_iterator<MapT, KeyT, MappedT>,
            std::pair<KeyT, MappedT>,
            forward_traversal_tag,
            std::pair<KeyT, MappedT> const
        > type;
    };

    template< class MapT, class KeyT, class MappedT >
    struct mfc_map_iterator :
        mfc_map_iterator_super<MapT, KeyT, MappedT>::type
    {
    private:
        typedef mfc_map_iterator self_t;
        typedef typename mfc_map_iterator_super<MapT, KeyT, MappedT>::type super_t;
        typedef typename super_t::reference ref_t;

    public:
        explicit mfc_map_iterator()
        { }

        explicit mfc_map_iterator(MapT const& map, POSITION pos) :
            m_pmap(boost::addressof(map)), m_posNext(pos)
        {
            increment();
        }

        explicit mfc_map_iterator(MapT const& map) :
            m_pmap(&map), m_pos(0) // end iterator
        { }

    template< class, class, class > friend struct mfc_map_iterator;
        template< class MapT_, class KeyT_, class MappedT_>
        mfc_map_iterator(mfc_map_iterator<MapT_, KeyT_, MappedT_> const& other) :
            m_pmap(other.m_pmap),
            m_pos(other.m_pos), m_posNext(other.m_posNext),
            m_key(other.m_key), m_mapped(other.m_mapped)
        { }

    private:
        MapT const *m_pmap;
        POSITION m_pos, m_posNext;
        KeyT m_key; MappedT m_mapped;

    friend class iterator_core_access;
        ref_t dereference() const
        {
            BOOST_ASSERT(m_pos != 0 && "out of range");
            return std::make_pair(m_key, m_mapped);
        }

        void increment()
        {
            BOOST_ASSERT(m_pos != 0 && "out of range");

            if (m_posNext == 0) {
                m_pos = 0;
                return;
            }

            m_pos = m_posNext;
            m_pmap->GetNextAssoc(m_posNext, m_key, m_mapped);
        }

        bool equal(self_t const& other) const
        {
            BOOST_ASSERT(m_pmap == other.m_pmap && "iterators incompatible");
            return m_pos == other.m_pos;
        }
    };

    struct mfc_map_functions
    {
        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            return Iterator(x, x.GetStartPosition());
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return Iterator(x);
        }
    };


#if !defined(BOOST_RANGE_MFC_NO_CPAIR)


    // mfc_cpair_map_iterator
    //
    // used by ::CMap and ::CMapStringToString
    //

    template< class MapT, class PairT >
    struct mfc_cpair_map_iterator;

    template< class MapT, class PairT >
    struct mfc_pget_map_iterator_super
    {
        typedef iterator_facade<
            mfc_cpair_map_iterator<MapT, PairT>,
            PairT,
            forward_traversal_tag
        > type;
    };

    template< class MapT, class PairT >
    struct mfc_cpair_map_iterator :
        mfc_pget_map_iterator_super<MapT, PairT>::type
    {
    private:
        typedef mfc_cpair_map_iterator self_t;
        typedef typename mfc_pget_map_iterator_super<MapT, PairT>::type super_t;
        typedef typename super_t::reference ref_t;

    public:
        explicit mfc_cpair_map_iterator()
        { }

        explicit mfc_cpair_map_iterator(MapT& map, PairT *pp) :
            m_pmap(boost::addressof(map)), m_pp(pp)
        { }

    template< class, class > friend struct mfc_cpair_map_iterator;
        template< class MapT_, class PairT_>
        mfc_cpair_map_iterator(mfc_cpair_map_iterator<MapT_, PairT_> const& other) :
            m_pmap(other.m_pmap), m_pp(other.m_pp)
        { }

    private:
        MapT  *m_pmap;
        PairT *m_pp;

    friend class iterator_core_access;
        ref_t dereference() const
        {
            BOOST_ASSERT(m_pp != 0 && "out of range");
            return *m_pp;
        }

        void increment()
        {
            BOOST_ASSERT(m_pp != 0 && "out of range");
            m_pp = m_pmap->PGetNextAssoc(m_pp);
        }

        bool equal(self_t const& other) const
        {
            BOOST_ASSERT(m_pmap == other.m_pmap && "iterators incompatible");
            return m_pp == other.m_pp;
        }
    };

    struct mfc_cpair_map_functions
    {
        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            // Workaround:
            // Assertion fails if empty.
            // MFC document is wrong.
    #if !defined(NDEBUG)
            if (x.GetCount() == 0) 
                return Iterator(x, 0);
    #endif

            return Iterator(x, x.PGetFirstAssoc());
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return Iterator(x, 0);
        }
    };


#endif // !defined(BOOST_RANGE_MFC_NO_CPAIR)


    // maps
    //

    template< >
    struct customization< ::CMapPtrToWord > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef void *key_t;
            typedef WORD mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    template< >
    struct customization< ::CMapPtrToPtr > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef void *key_t;
            typedef void *mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    template< >
    struct customization< ::CMapStringToOb > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef ::CString key_t;
            typedef ::CObject *mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    template< >
    struct customization< ::CMapStringToPtr > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef ::CString key_t;
            typedef void *mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    template< >
    struct customization< ::CMapStringToString > :
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
        mfc_cpair_map_functions
    #else
        mfc_map_functions
    #endif
    {
        template< class X >
        struct meta
        {
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
            typedef typename X::CPair pair_t;

            typedef mfc_cpair_map_iterator<X, pair_t> mutable_iterator;
            typedef mfc_cpair_map_iterator<X const, pair_t const> const_iterator;
    #else
            typedef ::CString key_t;
            typedef ::CString mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
    #endif
        };
    };


    template< >
    struct customization< ::CMapWordToOb > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef WORD key_t;
            typedef ::CObject *mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    template< >
    struct customization< ::CMapWordToPtr > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef WORD key_t;
            typedef void *mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    // templates
    //

    template< class Type, class ArgType >
    struct customization< ::CArray<Type, ArgType> > :
        array_functions
    {
        template< class X >
        struct meta
        {
            typedef Type val_t;

            typedef val_t *mutable_iterator;
            typedef val_t const *const_iterator;
        };
    };


    template< class Type, class ArgType >
    struct customization< ::CList<Type, ArgType> > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef Type val_t;

            typedef list_iterator<X, val_t> mutable_iterator;
    #if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
            typedef list_iterator<X const, val_t const> const_iterator;
    #else
            typedef list_iterator<X const, val_t const, val_t const> const_iterator;
    #endif
        };
    };


    template< class Key, class ArgKey, class Mapped, class ArgMapped >
    struct customization< ::CMap<Key, ArgKey, Mapped, ArgMapped> > :
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
        mfc_cpair_map_functions
    #else
        mfc_map_functions
    #endif
    {
        template< class X >
        struct meta
        {
    #if !defined(BOOST_RANGE_MFC_NO_CPAIR)
            typedef typename X::CPair pair_t;

            typedef mfc_cpair_map_iterator<X, pair_t> mutable_iterator;
            typedef mfc_cpair_map_iterator<X const, pair_t const> const_iterator;
    #else
            typedef Key key_t;
            typedef Mapped mapped_t;

            typedef mfc_map_iterator<X, key_t, mapped_t> mutable_iterator;
            typedef mutable_iterator const_iterator;
    #endif            
        };
    };


    template< class BaseClass, class PtrType >
    struct customization< ::CTypedPtrArray<BaseClass, PtrType> >
    {
        template< class X >
        struct fun
        {
            typedef typename remove_pointer<PtrType>::type val_t;

            typedef typename mpl::if_< is_const<X>,
                val_t const,
                val_t
            >::type val_t_;

            typedef val_t_ * const result_type;

            template< class PtrType_ >
            result_type operator()(PtrType_ p) const
            {
                return static_cast<result_type>(p);
            }
        };

        template< class X >
        struct meta
        {
            typedef typename compatible_mutable_iterator<BaseClass>::type miter_t;
            typedef typename range_const_iterator<BaseClass>::type citer_t;

            typedef transform_iterator<fun<X>, miter_t> mutable_iterator;
            typedef transform_iterator<fun<X const>, citer_t> const_iterator;
        };

        template< class Iterator, class X >
        Iterator begin(X& x)
        {
            return Iterator(boost::begin<BaseClass>(x), fun<X>());
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return Iterator(boost::end<BaseClass>(x), fun<X>());
        }
    };


    template< class BaseClass, class PtrType >
    struct customization< ::CTypedPtrList<BaseClass, PtrType> > :
        list_functions
    {
        template< class X >
        struct meta
        {
            typedef typename remove_pointer<PtrType>::type val_t;

            // not l-value
            typedef list_iterator<X, val_t * const, val_t * const> mutable_iterator;
            typedef list_iterator<X const, val_t const * const, val_t const * const> const_iterator;
        };
    };


    template< class BaseClass, class KeyPtrType, class MappedPtrType >
    struct customization< ::CTypedPtrMap<BaseClass, KeyPtrType, MappedPtrType> > :
        mfc_map_functions
    {
        template< class X >
        struct meta
        {
            typedef mfc_map_iterator<X, KeyPtrType, MappedPtrType> mutable_iterator;
            typedef mutable_iterator const_iterator;
        };
    };


    // strings
    //

#if defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)

    template< >
    struct customization< ::CString >
    {
        template< class X >
        struct meta
        {
            // LPTSTR/LPCTSTR is not always defined in <tchar.h>.
            typedef TCHAR *mutable_iterator;
            typedef TCHAR const *const_iterator;
        };

        template< class Iterator, class X >
        typename mutable_<Iterator, X>::type begin(X& x)
        {
            return x.GetBuffer(0);
        }

        template< class Iterator, class X >
        Iterator begin(X const& x)
        {
            return x;
        }

        template< class Iterator, class X >
        Iterator end(X& x)
        {
            return begin<Iterator>(x) + x.GetLength();
        }
    };

#endif // defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)


} } // namespace boost::range_detail_microsoft




// range customizations
//


// arrays
//
BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CByteArray
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CDWordArray
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CStringArray
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CUIntArray
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CWordArray
)


// lists
//
BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CObList
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CPtrList
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CStringList
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CObArray
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CPtrArray
)


// maps
//
BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapPtrToWord
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapPtrToPtr
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapStringToOb
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapStringToPtr
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapStringToString
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapWordToOb
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMapWordToPtr
)


// templates
//
BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CArray, 2
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CList, 2
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CMap, 4
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CTypedPtrArray, 2
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CTypedPtrList, 2
)

BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TEMPLATE(
    boost::range_detail_microsoft::using_type_as_tag,
    BOOST_PP_NIL, CTypedPtrMap, 3
)


// strings
//
#if defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)

    BOOST_RANGE_DETAIL_MICROSOFT_CUSTOMIZATION_TYPE(
        boost::range_detail_microsoft::using_type_as_tag,
        BOOST_PP_NIL, CString
    )

#endif




#endif
