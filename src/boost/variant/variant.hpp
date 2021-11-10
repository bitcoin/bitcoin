//-----------------------------------------------------------------------------
// boost variant/variant.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003 Eric Friedman, Itay Maman
// Copyright (c) 2012-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Thanks to Adam Romanek for providing patches for exception-disabled env.

#ifndef BOOST_VARIANT_VARIANT_HPP
#define BOOST_VARIANT_VARIANT_HPP

#include <cstddef> // for std::size_t
#include <new> // for placement new

#include <boost/type_index.hpp>

#include <boost/variant/detail/config.hpp>
#include <boost/mpl/aux_/value_wknd.hpp>

#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/detail/backup_holder.hpp>
#include <boost/variant/detail/enable_recursive_fwd.hpp>
#include <boost/variant/detail/forced_return.hpp>
#include <boost/variant/detail/initializer.hpp>
#include <boost/variant/detail/make_variant_list.hpp>
#include <boost/variant/detail/over_sequence.hpp>
#include <boost/variant/detail/visitation_impl.hpp>
#include <boost/variant/detail/hash_variant.hpp>
#include <boost/variant/detail/std_hash.hpp>

#include <boost/variant/detail/move.hpp>

#include <boost/detail/reference_content.hpp>
#include <boost/blank.hpp>
#include <boost/integer/common_factor_ct.hpp>
#include <boost/static_assert.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/has_nothrow_constructor.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>
#include <boost/type_traits/is_nothrow_move_assignable.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_rvalue_reference.hpp>
#include <boost/type_traits/is_constructible.hpp>
#include <boost/type_traits/add_lvalue_reference.hpp>
#include <boost/type_traits/declval.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/static_visitor.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/iterator_range.hpp>
#include <boost/mpl/iter_fold_if.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/protect.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/same_as.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/transform.hpp>

///////////////////////////////////////////////////////////////////////////////
// Implementation Macros:
//
// BOOST_VARIANT_VISITATION_UNROLLING_LIMIT
//   Defined in boost/variant/detail/visitation_impl.hpp.
//
// BOOST_VARIANT_MINIMIZE_SIZE
//   When #defined, implementation employs all known means to minimize the
//   size of variant obje   cts. However, often unsuccessful due to alignment
//   issues, and potentially harmful to runtime speed, so not enabled by
//   default. (TODO: Investigate further.)

#if defined(BOOST_VARIANT_MINIMIZE_SIZE)
#   include <climits> // for SCHAR_MAX
#   include <boost/mpl/eval_if.hpp>
#   include <boost/mpl/equal_to.hpp>
#   include <boost/mpl/identity.hpp>
#   include <boost/mpl/int.hpp>
#   include <boost/mpl/if.hpp>
#   include <boost/mpl/less.hpp>
#   include <boost/mpl/long.hpp>
#   include <boost/mpl/O1_size.hpp>
#endif


namespace boost {

namespace detail { namespace variant {

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction max_value
//
// Finds the maximum value of the unary metafunction F over Sequence.
//
template <typename Sequence, typename F>
struct max_value
{
private: // helpers, for metafunction result (below)

    typedef typename mpl::transform1<Sequence, F>::type transformed_;
    typedef typename mpl::max_element<transformed_

        >::type max_it;

public: // metafunction result

    typedef typename mpl::deref<max_it>::type
        type;

};

struct add_alignment
{
    template <typename State, typename Item>
    struct apply
        : mpl::size_t<
              ::boost::integer::static_lcm<
                  BOOST_MPL_AUX_VALUE_WKND(State)::value
                , ::boost::alignment_of<Item>::value
                >::value
            >
    {};
};

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction find_fallback_type
//
// Provides a fallback (i.e., nothrow default-constructible) type from the
// specified sequence, or no_fallback_type if not found.
//
// This implementation is designed to prefer boost::blank over other potential
// fallback types, regardless of its position in the specified sequence.
//

class no_fallback_type;

struct find_fallback_type_pred
{
    template <typename Iterator>
    struct apply
    {
    private:
        typedef typename mpl::deref<Iterator>::type t_;

    public:
        typedef mpl::not_< has_nothrow_constructor<t_> > type;
    };
};

template <typename Types>
struct find_fallback_type
{
private: // helpers, for metafunction result (below)

    typedef typename mpl::end<Types>::type end_it;

    // [Find the first suitable fallback type...]

    typedef typename mpl::iter_fold_if<
          Types
        , mpl::int_<0>, mpl::protect< mpl::next<> >
        , mpl::protect< find_fallback_type_pred >
        >::type first_result_;

    typedef typename first_result_::first first_result_index;
    typedef typename first_result_::second first_result_it;

    // [...now search the rest of the sequence for boost::blank...]

    typedef typename mpl::iter_fold_if<
          mpl::iterator_range< first_result_it,end_it >
        , first_result_index, mpl::protect< mpl::next<> >
        , mpl::protect< mpl::not_same_as<boost::blank> >
        >::type second_result_;

    typedef typename second_result_::second second_result_it;

public: // metafunction result

    // [...and return the results of the search:]
    typedef typename mpl::eval_if<
          is_same< second_result_it,end_it >
        , mpl::if_<
              is_same< first_result_it,end_it >
            , mpl::pair< no_fallback_type,no_fallback_type >
            , first_result_
            >
        , mpl::identity< second_result_ >
        >::type type;

};

#ifndef BOOST_NO_CXX11_NOEXCEPT
///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction is_variant_move_noexcept_constructible
//
// Returns true_type if all the types are nothrow move constructible.
//
template <class Types>
struct is_variant_move_noexcept_constructible {
    typedef typename boost::mpl::find_if<
        Types, mpl::not_<boost::is_nothrow_move_constructible<boost::mpl::_1> >
    >::type iterator_t;

    typedef typename boost::mpl::end<Types>::type end_t;
    typedef typename boost::is_same<
        iterator_t, end_t
    >::type type;
};

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction is_variant_move_noexcept_assignable
//
// Returns true_type if all the types are nothrow move constructible.
//
template <class Types>
struct is_variant_move_noexcept_assignable {
    typedef typename boost::mpl::find_if<
        Types, mpl::not_<boost::is_nothrow_move_assignable<boost::mpl::_1> >
    >::type iterator_t;

    typedef typename boost::mpl::end<Types>::type end_t;
    typedef typename boost::is_same<
        iterator_t, end_t
    >::type type;
};
#endif // BOOST_NO_CXX11_NOEXCEPT

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction is_variant_constructible_from
//
// Derives from true_type if at least one variant's type is constructible from T.
//
template <class T1, class T2>
struct is_constructible_ext:
    boost::mpl::or_<
        boost::is_constructible<
            T1,
            T2
        >,
        boost::is_constructible<
            T1,
            typename boost::add_lvalue_reference<T2>::type
        >
    >
{};

template <class T, class Types>
struct is_variant_constructible_from:
    boost::mpl::not_< boost::is_same<
        typename boost::mpl::find_if<
            Types,
            is_constructible_ext<boost::mpl::_1, T>
        >::type,
        typename boost::mpl::end<Types>::type
    > >
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), class Types>
struct is_variant_constructible_from< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Types >:
    boost::is_same<
        typename boost::mpl::find_if<
            typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::recursive_enabled_types,
            mpl::not_< is_variant_constructible_from< boost::mpl::_1, Types> >
        >::type,
        typename boost::mpl::end< typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::recursive_enabled_types >::type
    >
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), class Types>
struct is_variant_constructible_from< const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>& , Types >:
    is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Types >
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), class Types>
struct is_variant_constructible_from< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>& , Types >:
    is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Types >
{};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <BOOST_VARIANT_ENUM_PARAMS(typename T), class Types>
struct is_variant_constructible_from< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>&& , Types >:
    is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Types >
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), class Types>
struct is_variant_constructible_from< boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const && , Types >:
    is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Types >
{};

#endif // #ifndef BOOST_NO_CXX11_RVALUE_REFERENCE


///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction make_storage
//
// Provides an aligned storage type capable of holding any of the types
// specified in the given type-sequence.
//

template <typename Types, typename NeverUsesBackupFlag>
struct make_storage
{
private: // helpers, for metafunction result (below)

    typedef typename mpl::eval_if<
          NeverUsesBackupFlag
        , mpl::identity< Types >
        , mpl::push_front<
              Types, backup_holder<void*>
            >
        >::type types;

    typedef typename max_value<
          types, mpl::sizeof_<mpl::_1>
        >::type max_size;

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x0551))

    typedef typename mpl::fold<
          types
        , mpl::size_t<1>
        , add_alignment
        >::type max_alignment;

#else // borland

    // temporary workaround -- use maximal alignment
    typedef mpl::size_t< -1 > max_alignment;

#endif // borland workaround

public: // metafunction result

    typedef ::boost::aligned_storage<
          BOOST_MPL_AUX_VALUE_WKND(max_size)::value
        , BOOST_MPL_AUX_VALUE_WKND(max_alignment)::value
        > type;
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class destroyer
//
// Internal visitor that destroys the value it visits.
//
struct destroyer
    : public static_visitor<>
{
public: // visitor interfaces

    template <typename T>
    void internal_visit(T& operand, int) const BOOST_NOEXCEPT
    {
        operand.~T(); // must be noexcept

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x0551)) || \
    BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
        (void)operand; // suppresses warnings
#endif
    }

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class template known_get
//
// Visitor that returns a reference to content of the specified type.
//
// Precondition: visited variant MUST contain logical content of type T.
//
template <typename T>
class known_get
    : public static_visitor<T&>
{

public: // visitor interface

    T& operator()(T& operand) const BOOST_NOEXCEPT
    {
        return operand;
    }

    template <typename U>
    T& operator()(U&) const
    {
        // logical error to be here: see precondition above
        return ::boost::detail::variant::forced_return< T& >();
    }
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class copy_into
//
// Internal visitor that copies the value it visits into the given buffer.
//
class copy_into
    : public static_visitor<>
{
private: // representation

    void* storage_;

public: // structors

    explicit copy_into(void* storage) BOOST_NOEXCEPT
        : storage_(storage)
    {
    }

public: // internal visitor interface

    template <typename T>
    void internal_visit(boost::detail::variant::backup_holder<T>& operand, long) const
    {
        new(storage_) T( operand.get() );
    }

    template <typename T>
    void internal_visit(const boost::detail::variant::backup_holder<T>& operand, long) const
    {
        new(storage_) T( operand.get() );
    }

    template <typename T>
    void internal_visit(const T& operand, int) const
    {
        new(storage_) T(operand);
    }

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class move_into
//
// Internal visitor that moves the value it visits into the given buffer.
//
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
class move_into
    : public static_visitor<>
{
private: // representation

    void* storage_;

public: // structors

    explicit move_into(void* storage) BOOST_NOEXCEPT
        : storage_(storage)
    {
    }

public: // internal visitor interface

    template <typename T>
    void internal_visit(boost::detail::variant::backup_holder<T>& operand, long) const
    {
        new(storage_) T( ::boost::detail::variant::move(operand.get()) );
    }

    template <typename T>
    void internal_visit(T& operand, int) const BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(T(boost::declval<T>())))
    {
        new(storage_) T(::boost::detail::variant::move(operand));
    }
};
#endif

///////////////////////////////////////////////////////////////////////////////
// (detail) class assign_storage
//
// Internal visitor that assigns the given storage (which must be a
// constructed value of the same type) to the value it visits.
//
struct assign_storage
    : public static_visitor<>
{
private: // representation

    const void* rhs_storage_;

public: // structors

    explicit assign_storage(const void* rhs_storage) BOOST_NOEXCEPT
        : rhs_storage_(rhs_storage)
    {
    }

public: // internal visitor interfaces

    template <typename T>
    void internal_visit(backup_holder<T>& lhs_content, long) const
    {
        lhs_content.get()
            = static_cast< const backup_holder<T>* >(rhs_storage_)->get();
    }

    template <typename T>
    void internal_visit(const backup_holder<T>& lhs_content, long) const
    {
        lhs_content.get()
            = static_cast< const backup_holder<T>* >(rhs_storage_)->get();
    }

    template <typename T>
    void internal_visit(T& lhs_content, int) const
    {
        // NOTE TO USER :
        // Compile error here indicates one of variant's bounded types does
        // not meet the requirements of the Assignable concept. Thus,
        // variant is not Assignable.
        //
        // Hint: Are any of the bounded types const-qualified or references?
        //
        lhs_content = *static_cast< const T* >(rhs_storage_);
    }

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class move_storage
//
// Internal visitor that moves the given storage (which must be a
// constructed value of the same type) to the value it visits.
//
struct move_storage
    : public static_visitor<>
{
private: // representation

    void* rhs_storage_;

public: // structors

    explicit move_storage(void* rhs_storage) BOOST_NOEXCEPT
        : rhs_storage_(rhs_storage)
    {
    }

public: // internal visitor interfaces

    template <typename T>
    void internal_visit(backup_holder<T>& lhs_content, long) const
    {
        lhs_content.get()
            = ::boost::detail::variant::move(static_cast<backup_holder<T>* >(rhs_storage_)->get());
    }

    template <typename T>
    void internal_visit(const backup_holder<T>& lhs_content, long) const
    {
        lhs_content.get()
            = ::boost::detail::variant::move(static_cast<backup_holder<T>* >(rhs_storage_)->get());
    }

    template <typename T>
    void internal_visit(T& lhs_content, int) const
    {
        // NOTE TO USER :
        // Compile error here indicates one of variant's bounded types does
        // not meet the requirements of the Assignable concept. Thus,
        // variant is not Assignable.
        //
        // Hint: Are any of the bounded types const-qualified or references?
        //
        lhs_content = ::boost::detail::variant::move(*static_cast<T* >(rhs_storage_));
    }

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class direct_assigner
//
// Generic static visitor that: if and only if the visited value is of the
// specified type, assigns the given value to the visited value and returns
// true; else returns false.
//
template <typename T>
class direct_assigner
    : public static_visitor<bool>
{
private: // representation

    const T& rhs_;

public: // structors

    explicit direct_assigner(const T& rhs) BOOST_NOEXCEPT
        : rhs_(rhs)
    {
    }

public: // visitor interface

    bool operator()(T& lhs)
    {
        lhs = rhs_;
        return true;
    }

    template <typename U>
    bool operator()(U&) BOOST_NOEXCEPT
    {
        return false;
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
private:
    // silence MSVC warning C4512: assignment operator could not be generated
    direct_assigner& operator= (direct_assigner const&);
#endif
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class direct_mover
//
// Generic static visitor that: if and only if the visited value is of the
// specified type, move assigns the given value to the visited value and returns
// true; else returns false.
//
template <typename T>
class direct_mover
    : public static_visitor<bool>
{
private: // representation

    T& rhs_;

public: // structors

    explicit direct_mover(T& rhs) BOOST_NOEXCEPT
        : rhs_(rhs)
    {
    }

public: // visitor interface

    bool operator()(T& lhs)
    {
        lhs = ::boost::detail::variant::move(rhs_);
        return true;
    }

    template <typename U>
    bool operator()(U&) BOOST_NOEXCEPT
    {
        return false;
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
private:
    // silence MSVC warning C4512: assignment operator could not be generated
    direct_mover& operator= (direct_mover const&);
#endif
};


///////////////////////////////////////////////////////////////////////////////
// (detail) class backup_assigner
//
// Internal visitor that "assigns" the given value to the visited value,
// using backup to recover if the destroy-copy sequence fails.
//
// NOTE: This needs to be a friend of variant, as it needs access to
// indicate_which, indicate_backup_which, etc.
//
template <typename Variant>
class backup_assigner
    : public static_visitor<>
{
private: // representation

    Variant& lhs_;
    int rhs_which_;
    const void* rhs_content_;
    void (*copy_rhs_content_)(void*, const void*);

public: // structors

    template<class RhsT>
    backup_assigner(Variant& lhs, int rhs_which, const RhsT& rhs_content)
        : lhs_(lhs)
        , rhs_which_(rhs_which)
        , rhs_content_(&rhs_content)
        , copy_rhs_content_(&construct_impl<RhsT>)
    {
    }

private: // helpers, for visitor interface (below)

    template<class RhsT>
    static void construct_impl(void* addr, const void* obj)
    {
        new(addr) RhsT(*static_cast<const RhsT*>(obj));
    }

    template <typename LhsT>
    void backup_assign_impl(
          backup_holder<LhsT>& lhs_content
        , mpl::false_ // is_nothrow_move_constructible
        , long
        )
    {
        // Move lhs content to backup...
        backup_holder<LhsT> backup_lhs_content(0);
        backup_lhs_content.swap(lhs_content); // nothrow

        // ...destroy lhs content...
        lhs_content.~backup_holder<LhsT>(); // nothrow

        BOOST_TRY
        {
            // ...and attempt to copy rhs content into lhs storage:
            copy_rhs_content_(lhs_.storage_.address(), rhs_content_);
        }
        BOOST_CATCH (...)
        {
            // In case of failure, copy backup pointer to lhs storage...
            new(lhs_.storage_.address())
                    backup_holder<LhsT>( 0 ); // nothrow

            static_cast<backup_holder<LhsT>* >(lhs_.storage_.address())
                    ->swap(backup_lhs_content); // nothrow

            // ...and rethrow:
            BOOST_RETHROW;
        }
        BOOST_CATCH_END

        // In case of success, indicate new content type:
        lhs_.indicate_which(rhs_which_); // nothrow
    }

    template <typename LhsT>
    void backup_assign_impl(
          LhsT& lhs_content
        , mpl::true_ // is_nothrow_move_constructible
        , int
        )
    {
        // Move lhs content to backup...
        LhsT backup_lhs_content(
              ::boost::detail::variant::move(lhs_content)
            ); // nothrow

        // ...destroy lhs content...
        lhs_content.~LhsT(); // nothrow

        BOOST_TRY
        {
            // ...and attempt to copy rhs content into lhs storage:
            copy_rhs_content_(lhs_.storage_.address(), rhs_content_);
        }
        BOOST_CATCH (...)
        {
            // In case of failure, restore backup content to lhs storage...
            new(lhs_.storage_.address())
                LhsT(
                      ::boost::detail::variant::move(backup_lhs_content)
                    ); // nothrow

            // ...and rethrow:
            BOOST_RETHROW;
        }
        BOOST_CATCH_END

        // In case of success, indicate new content type:
        lhs_.indicate_which(rhs_which_); // nothrow
    }

    template <typename LhsT>
    void backup_assign_impl(
          LhsT& lhs_content
        , mpl::false_ // is_nothrow_move_constructible
        , int
        )
    {
        // Backup lhs content...
        LhsT* backup_lhs_ptr = new LhsT(lhs_content);

        // ...destroy lhs content...
        lhs_content.~LhsT(); // nothrow

        BOOST_TRY
        {
            // ...and attempt to copy rhs content into lhs storage:
            copy_rhs_content_(lhs_.storage_.address(), rhs_content_);
        }
        BOOST_CATCH (...)
        {
            // In case of failure, copy backup pointer to lhs storage...
            new(lhs_.storage_.address())
                backup_holder<LhsT>( backup_lhs_ptr ); // nothrow

            // ...indicate now using backup...
            lhs_.indicate_backup_which( lhs_.which() ); // nothrow

            // ...and rethrow:
            BOOST_RETHROW;
        }
        BOOST_CATCH_END

        // In case of success, indicate new content type...
        lhs_.indicate_which(rhs_which_); // nothrow

        // ...and delete backup:
        delete backup_lhs_ptr; // nothrow
    }

public: // visitor interface

    template <typename LhsT>
    void internal_visit(LhsT& lhs_content, int)
    {
        typedef typename is_nothrow_move_constructible<LhsT>::type
            nothrow_move;

        backup_assign_impl( lhs_content, nothrow_move(), 1L);
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
private:
    // silence MSVC warning C4512: assignment operator could not be generated
    backup_assigner& operator= (backup_assigner const&);
#endif
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class swap_with
//
// Visitor that swaps visited value with content of given variant.
//
// Precondition: Given variant MUST have same logical type as visited value.
//
template <typename Variant>
struct swap_with
    : public static_visitor<>
{
private: // representation

    Variant& toswap_;

public: // structors

    explicit swap_with(Variant& toswap) BOOST_NOEXCEPT
        : toswap_(toswap)
    {
    }

public: // internal visitor interfaces

    template <typename T>
    void operator()(T& operand) const
    {
        // Since the precondition ensures types are same, get T...
        known_get<T> getter;
        T& other = toswap_.apply_visitor(getter);

        // ...and swap:
        ::boost::detail::variant::move_swap( operand, other );
    }

private:
    swap_with& operator=(const swap_with&);

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class reflect
//
// Generic static visitor that performs a typeid on the value it visits.
//

class reflect
    : public static_visitor<const boost::typeindex::type_info&>
{
public: // visitor interfaces

    template <typename T>
    const boost::typeindex::type_info& operator()(const T&) const BOOST_NOEXCEPT
    {
        return boost::typeindex::type_id<T>().type_info();
    }

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class comparer
//
// Generic static visitor that compares the content of the given lhs variant
// with the visited rhs content using Comp.
//
// Precondition: lhs.which() == rhs.which()
//
template <typename Variant, typename Comp>
class comparer
    : public static_visitor<bool>
{
private: // representation

    const Variant& lhs_;

public: // structors

    explicit comparer(const Variant& lhs) BOOST_NOEXCEPT
        : lhs_(lhs)
    {
    }

public: // visitor interfaces

    template <typename T>
    bool operator()(T& rhs_content) const
    {
        // Since the precondition ensures lhs and rhs types are same, get T...
        known_get<T> getter;
        const T& lhs_content = lhs_.apply_visitor(getter);

        // ...and compare lhs and rhs contents:
        return Comp()(lhs_content, rhs_content);
    }

private:
    comparer& operator=(const comparer&);

};

///////////////////////////////////////////////////////////////////////////////
// (detail) class equal_comp
//
// Generic function object compares lhs with rhs using operator==.
//
struct equal_comp
{
    template <typename T>
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs == rhs;
    }
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class less_comp
//
// Generic function object compares lhs with rhs using operator<.
//
struct less_comp
{
    template <typename T>
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs < rhs;
    }
};

///////////////////////////////////////////////////////////////////////////////
// (detail) class template invoke_visitor
//
// Internal visitor that invokes the given visitor using:
//  * for wrappers (e.g., recursive_wrapper), the wrapper's held value.
//  * for all other values, the value itself.
//
template <typename Visitor, bool MoveSemantics>
class invoke_visitor
{
private: // representation

    Visitor& visitor_;

public: // visitor typedefs

    typedef typename Visitor::result_type
        result_type;

public: // structors

    explicit invoke_visitor(Visitor& visitor) BOOST_NOEXCEPT
        : visitor_(visitor)
    {
    }

public: // internal visitor interfaces

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

    //using workaround with is_same<T, T> to prenvent compilation error, because we need to use T in enable_if to make SFINAE work
    template <typename T>
    typename enable_if_c<MoveSemantics && is_same<T, T>::value, result_type>::type internal_visit(T&& operand, int)
    {
        return visitor_(::boost::move(operand));
    }

    //using workaround with is_same<T, T> to prenvent compilation error, because we need to use T in enable_if to make SFINAE work
    template <typename T>
    typename disable_if_c<MoveSemantics && is_same<T, T>::value, result_type>::type internal_visit(T&& operand, int)
    {
        return visitor_(operand);
    }

#else

    template <typename T>
    result_type internal_visit(T& operand, int)
    {
        return visitor_(operand);
    }

#   if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x0564))
    template <typename T>
    result_type internal_visit(const T& operand, int)
    {
        return visitor_(operand);
    }
#   endif //BORLAND

#endif //RVALUE REFERENCES

public: // internal visitor interfaces, cont.

    template <typename T>
    result_type internal_visit(boost::recursive_wrapper<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

    template <typename T>
    result_type internal_visit(const boost::recursive_wrapper<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

    template <typename T>
    result_type internal_visit(boost::detail::reference_content<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

    template <typename T>
    result_type internal_visit(const boost::detail::reference_content<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

    template <typename T>
    result_type internal_visit(boost::detail::variant::backup_holder<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

    template <typename T>
    result_type internal_visit(const boost::detail::variant::backup_holder<T>& operand, long)
    {
        return internal_visit( operand.get(), 1L );
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
private:
    // silence MSVC warning C4512: assignment operator could not be generated
    invoke_visitor& operator= (invoke_visitor const&);
#endif
};

}} // namespace detail::variant

///////////////////////////////////////////////////////////////////////////////
// class template variant (concept inspired by Andrei Alexandrescu)
//
// See docs and boost/variant/variant_fwd.hpp for more information.
//
template <
      typename T0_
    , BOOST_VARIANT_ENUM_SHIFTED_PARAMS(typename T)
    >
class variant
{
private: // helpers, for typedefs (below)

    typedef variant wknd_self_t;

    struct is_recursive_
        : detail::variant::is_recursive_flag<T0_>
    {
    };

    typedef typename mpl::eval_if<
          is_recursive_
        , T0_
        , mpl::identity< T0_ >
        >::type unwrapped_T0_;

    struct is_sequence_based_
        : detail::variant::is_over_sequence<unwrapped_T0_>
    {
    };

#if !defined(BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT)

private: // helpers, for typedefs (below)

    typedef typename mpl::eval_if<
          is_sequence_based_
        , unwrapped_T0_ // over_sequence<...>::type
        , detail::variant::make_variant_list<
              unwrapped_T0_
            , BOOST_VARIANT_ENUM_SHIFTED_PARAMS(T)
            >
        >::type specified_types;

    BOOST_STATIC_ASSERT((
          ::boost::mpl::not_< mpl::empty<specified_types> >::value
        ));

public: // public typedefs
    typedef typename mpl::eval_if<
          is_recursive_
        , mpl::transform<
              specified_types
            , mpl::protect<
                  detail::variant::quoted_enable_recursive<wknd_self_t>
                >
            >
        , mpl::identity< specified_types >
        >::type recursive_enabled_types;    // used by is_variant_constructible_from<> trait

    typedef typename mpl::transform<
          recursive_enabled_types
        , unwrap_recursive<mpl::_1>
        >::type types;

private: // internal typedefs

    typedef typename mpl::transform<
          recursive_enabled_types
        , mpl::protect< detail::make_reference_content<> >
        >::type internal_types;

    typedef typename mpl::front<
          internal_types
        >::type internal_T0;

#else // defined(BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT)

private: // helpers, for typedefs (below)

    typedef unwrapped_T0_ T0;

    #define BOOST_VARIANT_AUX_ENABLE_RECURSIVE_TYPEDEFS(z,N,_) \
        typedef typename mpl::eval_if< \
              is_recursive_ \
            , detail::variant::enable_recursive< \
                  BOOST_PP_CAT(T,N) \
                , wknd_self_t \
                > \
            , mpl::identity< BOOST_PP_CAT(T,N) > \
            >::type BOOST_PP_CAT(recursive_enabled_T,N); \
        /**/

    BOOST_PP_REPEAT(
          BOOST_VARIANT_LIMIT_TYPES
        , BOOST_VARIANT_AUX_ENABLE_RECURSIVE_TYPEDEFS
        , _
        )

    #undef BOOST_VARIANT_AUX_ENABLE_RECURSIVE_TYPEDEFS

    #define BOOST_VARIANT_AUX_UNWRAP_RECURSIVE_TYPEDEFS(z,N,_) \
        typedef typename unwrap_recursive< \
              BOOST_PP_CAT(recursive_enabled_T,N) \
            >::type BOOST_PP_CAT(public_T,N); \
        /**/

    BOOST_PP_REPEAT(
          BOOST_VARIANT_LIMIT_TYPES
        , BOOST_VARIANT_AUX_UNWRAP_RECURSIVE_TYPEDEFS
        , _
        )

    #undef BOOST_VARIANT_AUX_UNWRAP_RECURSIVE_TYPEDEFS

public: // public typedefs

    typedef typename detail::variant::make_variant_list<
          BOOST_VARIANT_ENUM_PARAMS(public_T)
        >::type types;

private: // helpers, for internal typedefs (below)

    #define BOOST_VARIANT_AUX_MAKE_REFERENCE_CONTENT_TYPEDEFS(z,N,_) \
        typedef detail::make_reference_content< \
              BOOST_PP_CAT(recursive_enabled_T,N) \
            >::type BOOST_PP_CAT(internal_T,N); \
        /**/

    BOOST_PP_REPEAT(
          BOOST_VARIANT_LIMIT_TYPES
        , BOOST_VARIANT_AUX_MAKE_REFERENCE_CONTENT_TYPEDEFS
        , _
        )

    #undef BOOST_VARIANT_AUX_MAKE_REFERENCE_CONTENT_TYPEDEFS

private: // internal typedefs

    typedef typename detail::variant::make_variant_list<
          BOOST_VARIANT_ENUM_PARAMS(internal_T)
        >::type internal_types;

private: // static precondition assertions

    // NOTE TO USER :
    // variant< type-sequence > syntax is not supported on this compiler!
    //
    BOOST_MPL_ASSERT_NOT(( is_sequence_based_ ));

#endif // BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT workaround

private: // helpers, for representation (below)

    typedef typename detail::variant::find_fallback_type<
          internal_types
        >::type fallback_type_result_;

    typedef typename fallback_type_result_::first
        fallback_type_index_;
    typedef typename fallback_type_result_::second
        fallback_type_;

    struct has_fallback_type_
        : mpl::not_<
              is_same< fallback_type_, detail::variant::no_fallback_type >
            >
    {
    };

    typedef has_fallback_type_
        never_uses_backup_flag;

    typedef typename detail::variant::make_storage<
          internal_types, never_uses_backup_flag
        >::type storage_t;

#ifndef BOOST_NO_CXX11_NOEXCEPT
    typedef typename detail::variant::is_variant_move_noexcept_constructible<
        internal_types
    > variant_move_noexcept_constructible;

    typedef typename detail::variant::is_variant_move_noexcept_assignable<
        internal_types
    > variant_move_noexcept_assignable;

#endif

private: // helpers, for representation (below)

    // which_ on:
    // * [0,  size<internal_types>) indicates stack content
    // * [-size<internal_types>, 0) indicates pointer to heap backup
    // if which_ >= 0:
    // * then which() -> which_
    // * else which() -> -(which_ + 1)

#if !defined(BOOST_VARIANT_MINIMIZE_SIZE)

    typedef int which_t;

#else // defined(BOOST_VARIANT_MINIMIZE_SIZE)

    // [if O1_size available, then attempt which_t size optimization...]
    // [select signed char if fewer than SCHAR_MAX types, else signed int:]
    typedef typename mpl::eval_if<
          mpl::equal_to< mpl::O1_size<internal_types>, mpl::long_<-1> >
        , mpl::identity< int >
        , mpl::if_<
              mpl::less< mpl::O1_size<internal_types>, mpl::int_<SCHAR_MAX> >
            , signed char
            , int
            >
        >::type which_t;

#endif // BOOST_VARIANT_MINIMIZE_SIZE switch

// representation -- private when possible
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    private:
#else
    public:
#endif

    which_t which_;
    storage_t storage_;

    void indicate_which(int which_arg) BOOST_NOEXCEPT
    {
        which_ = static_cast<which_t>( which_arg );
    }

    void indicate_backup_which(int which_arg) BOOST_NOEXCEPT
    {
        which_ = static_cast<which_t>( -(which_arg + 1) );
    }

private: // helpers, for queries (below)

    bool using_backup() const BOOST_NOEXCEPT
    {
        return which_ < 0;
    }

public: // queries

    int which() const BOOST_NOEXCEPT
    {
        // If using heap backup...
        if (using_backup())
            // ...then return adjusted which_:
            return -(which_ + 1);

        // Otherwise, return which_ directly:
        return which_;
    }

private: // helpers, for structors (below)

    struct initializer
        : BOOST_VARIANT_AUX_INITIALIZER_T(
              recursive_enabled_types, recursive_enabled_T
            )
    {
    };

    void destroy_content() BOOST_NOEXCEPT
    {
        detail::variant::destroyer visitor;
        this->internal_apply_visitor(visitor);
    }

public: // structors

    ~variant() BOOST_NOEXCEPT
    {
        destroy_content();
    }

    variant()
#if !(defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x5130))
              BOOST_NOEXCEPT_IF(boost::has_nothrow_constructor<internal_T0>::value)
#endif
    {
#ifdef _MSC_VER
#pragma warning( push )
// behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
#pragma warning( disable : 4345 )
#endif
        // NOTE TO USER :
        // Compile error from here indicates that the first bound
        // type is not default-constructible, and so variant cannot
        // support its own default-construction.
        //
        new( storage_.address() ) internal_T0();
        indicate_which(0); // zero is the index of the first bounded type
#ifdef _MSC_VER
#pragma warning( pop )
#endif
    }

private: // helpers, for structors, cont. (below)

    class convert_copy_into
        : public static_visitor<int>
    {
    private: // representation

        void* storage_;

    public: // structors

        explicit convert_copy_into(void* storage) BOOST_NOEXCEPT
            : storage_(storage)
        {
        }

    public: // internal visitor interfaces (below)

        template <typename T>
        int internal_visit(T& operand, int) const
        {
            // NOTE TO USER :
            // Compile error here indicates one of the source variant's types
            // cannot be unambiguously converted to the destination variant's
            // types (or that no conversion exists).
            //
            return initializer::initialize(storage_, operand);
        }

#   if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x0564))
        template <typename T>
        result_type internal_visit(const T& operand, int) const
        {
            return initializer::initialize(storage_, operand);
        }
#   endif

        template <typename T>
        int internal_visit(boost::detail::reference_content<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::detail::reference_content<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(boost::detail::variant::backup_holder<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::detail::variant::backup_holder<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(boost::recursive_wrapper<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::recursive_wrapper<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

    };

    friend class convert_copy_into;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    class convert_move_into
        : public static_visitor<int>
    {
    private: // representation

        void* storage_;

    public: // structors

        explicit convert_move_into(void* storage) BOOST_NOEXCEPT
            : storage_(storage)
        {
        }

    public: // internal visitor interfaces (below)

        template <typename T>
        int internal_visit(T& operand, int) const
        {
            // NOTE TO USER :
            // Compile error here indicates one of the source variant's types
            // cannot be unambiguously converted to the destination variant's
            // types (or that no conversion exists).
            //
            return initializer::initialize(storage_, detail::variant::move(operand) );
        }

        template <typename T>
        int internal_visit(boost::detail::reference_content<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::detail::reference_content<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(boost::detail::variant::backup_holder<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::detail::variant::backup_holder<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(boost::recursive_wrapper<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }

        template <typename T>
        int internal_visit(const boost::recursive_wrapper<T>& operand, long) const
        {
            return internal_visit( operand.get(), 1L );
        }
    };

    friend class convert_move_into;
#endif

private: // helpers, for structors, below

    template <typename T>
    void convert_construct(
          T& operand
        , int
        , mpl::false_ = mpl::false_() // is_foreign_variant
        )
    {
        // NOTE TO USER :
        // Compile error here indicates that the given type is not
        // unambiguously convertible to one of the variant's types
        // (or that no conversion exists).
        //
        indicate_which(
              initializer::initialize(
                  storage_.address()
                , operand
                )
            );
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename T>
    typename boost::enable_if<boost::is_rvalue_reference<T&&> >::type convert_construct(
          T&& operand
        , int
        , mpl::false_ = mpl::false_() // is_foreign_variant
        )
    {
        // NOTE TO USER :
        // Compile error here indicates that the given type is not
        // unambiguously convertible to one of the variant's types
        // (or that no conversion exists).
        //
        indicate_which(
              initializer::initialize(
                  storage_.address()
                , detail::variant::move(operand)
                )
            );
    }
#endif

    template <typename Variant>
    void convert_construct(
          Variant& operand
        , long
        , mpl::true_// is_foreign_variant
        )
    {
        convert_copy_into visitor(storage_.address());
        indicate_which(
              operand.internal_apply_visitor(visitor)
            );
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename Variant>
    typename boost::enable_if<boost::is_rvalue_reference<Variant&&> >::type convert_construct(
          Variant&& operand
        , long
        , mpl::true_// is_foreign_variant
        )
    {
        convert_move_into visitor(storage_.address());
        indicate_which(
              operand.internal_apply_visitor(visitor)
            );
    }
#endif

    template <typename Variant>
    void convert_construct_variant(Variant& operand)
    {
        // [Determine if the given variant is itself a bounded type, or if its
        //  content needs to be converted (i.e., it is a 'foreign' variant):]
        //

        typedef typename mpl::find_if<
              types
            , is_same<
                  add_const<mpl::_1>
                , const Variant
                >
            >::type found_it;

        typedef typename mpl::end<types>::type not_found;
        typedef typename is_same<
              found_it, not_found
            >::type is_foreign_variant;

        // Convert construct from operand:
        convert_construct(
              operand, 1L
            , is_foreign_variant()
            );
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename Variant>
    typename boost::enable_if<boost::is_rvalue_reference<Variant&&> >::type convert_construct_variant(Variant&& operand)
    {
        // [Determine if the given variant is itself a bounded type, or if its
        //  content needs to be converted (i.e., it is a 'foreign' variant):]
        //

        typedef typename mpl::find_if<
              types
            , is_same<
                  add_const<mpl::_1>
                , const Variant
                >
            >::type found_it;

        typedef typename mpl::end<types>::type not_found;
        typedef typename is_same<
              found_it, not_found
            >::type is_foreign_variant;

        // Convert move construct from operand:
        convert_construct(
              detail::variant::move(operand), 1L
            , is_foreign_variant()
            );
    }
#endif

    template <BOOST_VARIANT_ENUM_PARAMS(typename U)>
    typename boost::enable_if<mpl::or_<
        boost::is_same<boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>, variant>,
        boost::detail::variant::is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>&, internal_types>
    > >::type convert_construct(
          boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>& operand
        , long
        )
    {
        convert_construct_variant(operand);
    }

    template <BOOST_VARIANT_ENUM_PARAMS(typename U)>
    typename boost::enable_if<mpl::or_<
        boost::is_same<boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>, variant>,
        boost::detail::variant::is_variant_constructible_from<const boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>&, internal_types>
    > >::type convert_construct(
          const boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>& operand
        , long
        )
    {
        convert_construct_variant(operand);
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <BOOST_VARIANT_ENUM_PARAMS(typename U)>
    typename boost::enable_if<mpl::or_<
        boost::is_same<boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>, variant>,
        boost::detail::variant::is_variant_constructible_from<boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>&&, internal_types>
    > >::type convert_construct(
          boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>&& operand
        , long
        )
    {
        convert_construct_variant( detail::variant::move(operand) );
    }
#endif

public: // structors, cont.

    template <typename T>
    variant(const T& operand,
        typename boost::enable_if<mpl::or_<
            mpl::and_<
                mpl::not_< boost::is_same<T, variant> >,
                boost::detail::variant::is_variant_constructible_from<const T&, internal_types>
            >,
            boost::is_same<T, boost::recursive_variant_> >,
            bool >::type = true)
    {
        convert_construct(operand, 1L);
    }

    template <typename T>
    variant(
          T& operand
        , typename boost::enable_if<mpl::or_<
            mpl::and_<
                mpl::not_< is_const<T> >,
                mpl::not_< boost::is_same<T, variant> >,
                boost::detail::variant::is_variant_constructible_from<T&, internal_types>
            >,
            boost::is_same<T, boost::recursive_variant_> >,
            bool >::type = true
        )
    {
        convert_construct(operand, 1L);
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <class T>
    variant(T&& operand,
        typename boost::enable_if<mpl::or_<
            mpl::and_<
                boost::is_rvalue_reference<T&&>,
                mpl::not_< boost::is_const<T> >,
                mpl::not_< boost::is_same<T, variant> >,
                boost::detail::variant::is_variant_constructible_from<T&&, internal_types>
            >,
            boost::is_same<T, boost::recursive_variant_> >,
            bool >::type = true)
    {
        convert_construct( detail::variant::move(operand), 1L);
    }
#endif

public: // structors, cont.

    // [MSVC6 requires copy constructor appear after template constructors]
    variant(const variant& operand)
    {
        // Copy the value of operand into *this...
        detail::variant::copy_into visitor( storage_.address() );
        operand.internal_apply_visitor(visitor);

        // ...and activate the *this's primary storage on success:
        indicate_which(operand.which());
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    variant(variant&& operand) BOOST_NOEXCEPT_IF(variant_move_noexcept_constructible::type::value)
    {
        // Move the value of operand into *this...
        detail::variant::move_into visitor( storage_.address() );
        operand.internal_apply_visitor(visitor);

        // ...and activate the *this's primary storage on success:
        indicate_which(operand.which());
    }
#endif

private: // helpers, for modifiers (below)

#   if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template <typename Variant>
    friend class detail::variant::backup_assigner;
#   endif

    // class assigner
    //
    // Internal visitor that "assigns" the visited value to the given variant
    // by appropriate destruction and copy-construction.
    //

    class assigner
        : public static_visitor<>
    {
    protected: // representation

        variant& lhs_;
        const int rhs_which_;

    public: // structors

        assigner(variant& lhs, int rhs_which) BOOST_NOEXCEPT
            : lhs_(lhs)
            , rhs_which_(rhs_which)
        {
        }

    protected: // helpers, for internal visitor interface (below)

        template <typename RhsT, typename B1, typename B2>
        void assign_impl(
              const RhsT& rhs_content
            , mpl::true_ // has_nothrow_copy
            , B1 // is_nothrow_move_constructible
            , B2 // has_fallback_type
            ) const BOOST_NOEXCEPT
        {
            // Destroy lhs's content...
            lhs_.destroy_content(); // nothrow

            // ...copy rhs content into lhs's storage...
            new(lhs_.storage_.address())
                RhsT( rhs_content ); // nothrow

            // ...and indicate new content type:
            lhs_.indicate_which(rhs_which_); // nothrow
        }

        template <typename RhsT, typename B>
        void assign_impl(
              const RhsT& rhs_content
            , mpl::false_ // has_nothrow_copy
            , mpl::true_ // is_nothrow_move_constructible
            , B // has_fallback_type
            ) const
        {
            // Attempt to make a temporary copy (so as to move it below)...
            RhsT temp(rhs_content);

            // ...and upon success destroy lhs's content...
            lhs_.destroy_content(); // nothrow

            // ...move the temporary copy into lhs's storage...
            new(lhs_.storage_.address())
                RhsT( detail::variant::move(temp) ); // nothrow

            // ...and indicate new content type:
            lhs_.indicate_which(rhs_which_); // nothrow
        }

        void construct_fallback() const BOOST_NOEXCEPT {
            // In case of failure, default-construct fallback type in lhs's storage...
            new (lhs_.storage_.address())
                fallback_type_; // nothrow

            // ...indicate construction of fallback type...
            lhs_.indicate_which(
                  BOOST_MPL_AUX_VALUE_WKND(fallback_type_index_)::value
                ); // nothrow
        }

        template <typename RhsT>
        void assign_impl(
              const RhsT& rhs_content
            , mpl::false_ // has_nothrow_copy
            , mpl::false_ // is_nothrow_move_constructible
            , mpl::true_ // has_fallback_type
            ) const
        {
            // Destroy lhs's content...
            lhs_.destroy_content(); // nothrow

            BOOST_TRY
            {
                // ...and attempt to copy rhs's content into lhs's storage:
                new(lhs_.storage_.address())
                    RhsT( rhs_content );
            }
            BOOST_CATCH (...)
            {
                construct_fallback();

                // ...and rethrow:
                BOOST_RETHROW;
            }
            BOOST_CATCH_END

            // In the event of success, indicate new content type:
            lhs_.indicate_which(rhs_which_); // nothrow
        }

        template <typename RhsT>
        void assign_impl(
              const RhsT& rhs_content
            , mpl::false_ // has_nothrow_copy
            , mpl::false_ // is_nothrow_move_constructible
            , mpl::false_ // has_fallback_type
            ) const
        {
            detail::variant::backup_assigner<wknd_self_t>
                visitor(lhs_, rhs_which_, rhs_content);
            lhs_.internal_apply_visitor(visitor);
        }

    public: // internal visitor interfaces

        template <typename RhsT>
        void internal_visit(const RhsT& rhs_content, int) const
        {
            typedef typename has_nothrow_copy<RhsT>::type
                nothrow_copy;
            typedef typename mpl::or_< // reduces compile-time
                  nothrow_copy
                , is_nothrow_move_constructible<RhsT>
                >::type nothrow_move_constructor;

            assign_impl(
                  rhs_content
                , nothrow_copy()
                , nothrow_move_constructor()
                , has_fallback_type_()
                );
        }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        assigner& operator= (assigner const&);
#endif
    };

    friend class assigner;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    // class move_assigner
    //
    // Internal visitor that "move assigns" the visited value to the given variant
    // by appropriate destruction and move-construction.
    //

    class move_assigner
        : public assigner
    {
    public: // structors

        move_assigner(variant& lhs, int rhs_which) BOOST_NOEXCEPT
            : assigner(lhs, rhs_which)
        {
        }

    private: // helpers, for internal visitor interface (below)

        template <typename RhsT, typename B2>
        void assign_impl(
              RhsT& rhs_content
            , mpl::true_ // has_nothrow_copy
            , mpl::false_ // is_nothrow_move_constructible
            , B2 // has_fallback_type
            ) const BOOST_NOEXCEPT
        {
            assigner::assign_impl(rhs_content, mpl::true_(), mpl::false_(), B2());
        }

        template <typename RhsT, typename B, typename B2>
        void assign_impl(
              RhsT& rhs_content
            , B // has_nothrow_copy
            , mpl::true_ // is_nothrow_move_constructible
            , B2 // has_fallback_type
            ) const BOOST_NOEXCEPT
        {
            // ...destroy lhs's content...
            assigner::lhs_.destroy_content(); // nothrow

            // ...move the rhs_content into lhs's storage...
            new(assigner::lhs_.storage_.address())
                RhsT( detail::variant::move(rhs_content) ); // nothrow

            // ...and indicate new content type:
            assigner::lhs_.indicate_which(assigner::rhs_which_); // nothrow
        }

        template <typename RhsT>
        void assign_impl(
              RhsT& rhs_content
            , mpl::false_ // has_nothrow_copy
            , mpl::false_ // is_nothrow_move_constructible
            , mpl::true_ // has_fallback_type
            ) const
        {
            // Destroy lhs's content...
            assigner::lhs_.destroy_content(); // nothrow

            BOOST_TRY
            {
                // ...and attempt to copy rhs's content into lhs's storage:
                new(assigner::lhs_.storage_.address())
                    RhsT( detail::variant::move(rhs_content) );
            }
            BOOST_CATCH (...)
            {
                assigner::construct_fallback();

                // ...and rethrow:
                BOOST_RETHROW;
            }
            BOOST_CATCH_END

            // In the event of success, indicate new content type:
            assigner::lhs_.indicate_which(assigner::rhs_which_); // nothrow
        }

        template <typename RhsT>
        void assign_impl(
              RhsT& rhs_content
            , mpl::false_ // has_nothrow_copy
            , mpl::false_ // is_nothrow_move_constructible
            , mpl::false_ // has_fallback_type
            ) const
        {
            assigner::assign_impl(rhs_content, mpl::false_(), mpl::false_(), mpl::false_());
        }

    public: // internal visitor interfaces

        template <typename RhsT>
        void internal_visit(RhsT& rhs_content, int) const
        {
            typedef typename is_nothrow_move_constructible<RhsT>::type
                nothrow_move_constructor;
            typedef typename mpl::or_< // reduces compile-time
                  nothrow_move_constructor
                , has_nothrow_copy<RhsT>
                >::type nothrow_copy;

            assign_impl(
                  rhs_content
                , nothrow_copy()
                , nothrow_move_constructor()
                , has_fallback_type_()
                );
        }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        move_assigner& operator= (move_assigner const&);
#endif
    };

    friend class move_assigner;
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    void variant_assign(const variant& rhs)
    {
        // If the contained types are EXACTLY the same...
        if (which_ == rhs.which_)
        {
            // ...then assign rhs's storage to lhs's content:
            detail::variant::assign_storage visitor(rhs.storage_.address());
            this->internal_apply_visitor(visitor);
        }
        else
        {
            // Otherwise, perform general (copy-based) variant assignment:
            assigner visitor(*this, rhs.which());
            rhs.internal_apply_visitor(visitor);
        }
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void variant_assign(variant&& rhs)
    {
        // If the contained types are EXACTLY the same...
        if (which_ == rhs.which_)
        {
            // ...then move rhs's storage to lhs's content:
            detail::variant::move_storage visitor(rhs.storage_.address());
            this->internal_apply_visitor(visitor);
        }
        else
        {
            // Otherwise, perform general (move-based) variant assignment:
            move_assigner visitor(*this, rhs.which());
            rhs.internal_apply_visitor(visitor);
        }
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

private: // helpers, for modifiers (below)

    template <typename T>
    void assign(const T& rhs)
    {
        // If direct T-to-T assignment is not possible...
        detail::variant::direct_assigner<T> direct_assign(rhs);
        if (this->apply_visitor(direct_assign) == false)
        {
            // ...then convert rhs to variant and assign:
            //
            // While potentially inefficient, the following construction of a
            // variant allows T as any type convertible to one of the bounded
            // types without excessive code redundancy.
            //
            variant temp(rhs);
            variant_assign( detail::variant::move(temp) );
        }
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename T>
    void move_assign(T&& rhs)
    {
        // If direct T-to-T move assignment is not possible...
        detail::variant::direct_mover<T> direct_move(rhs);
        if (this->apply_visitor(direct_move) == false)
        {
            // ...then convert rhs to variant and assign:
            //
            // While potentially inefficient, the following construction of a
            // variant allows T as any type convertible to one of the bounded
            // types without excessive code redundancy.
            //
            variant temp( detail::variant::move(rhs) );
            variant_assign( detail::variant::move(temp) );
        }
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

public: // modifiers

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    template <class T>
    typename boost::enable_if<
        boost::mpl::and_<
            boost::is_rvalue_reference<T&&>,
            mpl::not_< boost::is_const<T> >,
            boost::detail::variant::is_variant_constructible_from<T&&, internal_types>
        >,
        variant&
    >::type operator=(T&& rhs)
    {
        move_assign( detail::variant::move(rhs) );
        return *this;
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    template <typename T>
    typename boost::enable_if<
        mpl::or_<
            boost::is_same<T, variant>,
            boost::detail::variant::is_variant_constructible_from<const T&, internal_types>
        >,
        variant&
    >::type operator=(const T& rhs)
    {
        assign(rhs);
        return *this;
    }

    // [MSVC6 requires copy assign appear after templated operator=]
    variant& operator=(const variant& rhs)
    {
        variant_assign(rhs);
        return *this;
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    variant& operator=(variant&& rhs)
#if !defined(__GNUC__) || (__GNUC__ != 4) || (__GNUC_MINOR__ > 6) || defined(__clang__)
        BOOST_NOEXCEPT_IF(variant_move_noexcept_constructible::type::value && variant_move_noexcept_assignable::type::value)
#endif
    {
        variant_assign( detail::variant::move(rhs) );
        return *this;
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    void swap(variant& rhs)
    {
        // If the contained types are the same...
        if (which() == rhs.which())
        {
            // ...then swap the values directly:
            detail::variant::swap_with<variant> visitor(rhs);
            this->apply_visitor(visitor);
        }
        else
        {
            // ...otherwise, perform general variant swap:
            variant tmp( detail::variant::move(rhs) );
            rhs = detail::variant::move(*this);
            *this = detail::variant::move(tmp);
        }
    }

public: // queries

    //
    // NOTE: member which() defined above.
    //

    bool empty() const BOOST_NOEXCEPT
    {
        return false;
    }

    const boost::typeindex::type_info& type() const
    {
        detail::variant::reflect visitor;
        return this->apply_visitor(visitor);
    }

public: // prevent comparison with foreign types

    template <typename U>
    void operator==(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

    template <typename U>
    void operator<(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

    template <typename U>
    void operator!=(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

    template <typename U>
    void operator>(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

    template <typename U>
    void operator<=(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

    template <typename U>
    void operator>=(const U&) const
    {
        BOOST_STATIC_ASSERT( false && sizeof(U) );
    }

public: // comparison operators

    // [MSVC6 requires these operators appear after template operators]

    bool operator==(const variant& rhs) const
    {
        if (this->which() != rhs.which())
            return false;

        detail::variant::comparer<
              variant, detail::variant::equal_comp
            > visitor(*this);
        return rhs.apply_visitor(visitor);
    }

    bool operator<(const variant& rhs) const
    {
        //
        // Dirk Schreib suggested this collating order.
        //

        if (this->which() != rhs.which())
            return this->which() < rhs.which();

        detail::variant::comparer<
              variant, detail::variant::less_comp
            > visitor(*this);
        return rhs.apply_visitor(visitor);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // comparison operators != > <= >=
    inline bool operator!=(const variant& rhs) const
    {
        return !(*this == rhs);
    }

    inline bool operator>(const variant& rhs) const
    {
        return rhs < *this;
    }

    inline bool operator<=(const variant& rhs) const
    {
        return !(*this > rhs);
    }

    inline bool operator>=(const variant& rhs) const
    {
        return !(*this < rhs);
    }

// helpers, for visitation support (below) -- private when possible
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)

    template < BOOST_VARIANT_ENUM_PARAMS(typename U) >
    friend class variant;

private:

#else// defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)

public:

#endif// !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)

    template <typename Visitor, typename VoidPtrCV>
    BOOST_FORCEINLINE static typename Visitor::result_type
    internal_apply_visitor_impl(
          int internal_which
        , int logical_which
        , Visitor& visitor
        , VoidPtrCV storage
        )
    {
        typedef mpl::int_<0> first_which;
        typedef typename mpl::begin<internal_types>::type first_it;
        typedef typename mpl::end<internal_types>::type last_it;

        typedef detail::variant::visitation_impl_step<
              first_it, last_it
            > first_step;

        return detail::variant::visitation_impl(
              internal_which, logical_which
            , visitor, storage, mpl::false_()
            , never_uses_backup_flag()
            , static_cast<first_which*>(0), static_cast<first_step*>(0)
            );
    }

    template <typename Visitor>
    BOOST_FORCEINLINE typename Visitor::result_type
    internal_apply_visitor(Visitor& visitor)
    {
        return internal_apply_visitor_impl(
              which_, which(), visitor, storage_.address()
            );
    }

    template <typename Visitor>
    BOOST_FORCEINLINE typename Visitor::result_type
    internal_apply_visitor(Visitor& visitor) const
    {
        return internal_apply_visitor_impl(
              which_, which(), visitor, storage_.address()
            );
    }

public: // visitation support

#ifndef BOOST_NO_CXX11_REF_QUALIFIERS

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor(Visitor& visitor) &&
    {
        detail::variant::invoke_visitor<Visitor, true> invoker(visitor);
        return this->internal_apply_visitor(invoker);
    }

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor(Visitor& visitor) const&&
    {
        detail::variant::invoke_visitor<Visitor, true> invoker(visitor);
        return this->internal_apply_visitor(invoker);
    }

#endif

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor(Visitor& visitor)
#ifndef BOOST_NO_CXX11_REF_QUALIFIERS
    &
#endif
    {
        detail::variant::invoke_visitor<Visitor, false> invoker(visitor);
        return this->internal_apply_visitor(invoker);
    }

    template <typename Visitor>
    typename Visitor::result_type
    apply_visitor(Visitor& visitor) const
#ifndef BOOST_NO_CXX11_REF_QUALIFIERS
    &
#endif
    {
        detail::variant::invoke_visitor<Visitor, false> invoker(visitor);
        return this->internal_apply_visitor(invoker);
    }

}; // class variant

///////////////////////////////////////////////////////////////////////////////
// metafunction make_variant_over
//
// See docs and boost/variant/variant_fwd.hpp for more information.
//
template <typename Types>
struct make_variant_over
{
private: // precondition assertions

    BOOST_STATIC_ASSERT(( ::boost::mpl::is_sequence<Types>::value ));
    typedef typename boost::mpl::insert_range<
      boost::mpl::list<>
    , boost::mpl::end< boost::mpl::list<> >::type
    , Types
    >::type copied_sequence_t;

public: // metafunction result

    typedef variant<
          detail::variant::over_sequence<copied_sequence_t>
        > type;

};

///////////////////////////////////////////////////////////////////////////////
// function template swap
//
// Swaps two variants of the same type (i.e., identical specification).
//
template < BOOST_VARIANT_ENUM_PARAMS(typename T) >
inline void swap(
      variant< BOOST_VARIANT_ENUM_PARAMS(T) >& lhs
    , variant< BOOST_VARIANT_ENUM_PARAMS(T) >& rhs
    )
{
    lhs.swap(rhs);
}

} // namespace boost

// implementation additions

#if !defined(BOOST_NO_IOSTREAM)
#include <boost/variant/detail/variant_io.hpp>
#endif // BOOST_NO_IOSTREAM

#endif // BOOST_VARIANT_VARIANT_HPP
