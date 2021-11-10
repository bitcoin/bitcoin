// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP
#define BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/offset_based_getter.hpp>
#include <boost/pfr/detail/fields_count.hpp>
#include <boost/pfr/detail/make_flat_tuple_of_references.hpp>
#include <boost/pfr/detail/make_integer_sequence.hpp>
#include <boost/pfr/detail/size_array.hpp>
#include <boost/pfr/detail/size_t_.hpp>
#include <boost/pfr/detail/rvalue_t.hpp>

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace boost { namespace pfr { namespace detail {

///////////////////// General utility stuff

template <class T> struct identity {
    typedef T type;
};

template <class T>
constexpr T construct_helper() noexcept { // adding const here allows to deal with copyable only types
    return {};
}

template <class T> constexpr size_array<sizeof(T) * 3> fields_count_and_type_ids_with_zeros() noexcept;
template <class T> constexpr auto flat_array_of_type_ids() noexcept;

///////////////////// All the stuff for representing Type as integer and converting integer back to type
namespace typeid_conversions {

///////////////////// Helper constants and typedefs

#ifdef _MSC_VER
#   pragma warning( push )
    // '<<': check operator precedence for possible error; use parentheses to clarify precedence
#   pragma warning( disable : 4554 ) 
#endif

constexpr std::size_t native_types_mask = 31;
constexpr std::size_t bits_per_extension = 3;
constexpr std::size_t extension_mask = (
    static_cast<std::size_t>((1 << bits_per_extension) - 1)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);
constexpr std::size_t native_ptr_type = (
    static_cast<std::size_t>(1)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);
constexpr std::size_t native_const_ptr_type = (
    static_cast<std::size_t>(2)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_const_volatile_ptr_type = (
    static_cast<std::size_t>(3)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_volatile_ptr_type = (
    static_cast<std::size_t>(4)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_ref_type = (
    static_cast<std::size_t>(5)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

template <std::size_t Index, std::size_t Extension>
using if_extension = std::enable_if_t< (Index & extension_mask) == Extension >*;

///////////////////// Helper functions
template <std::size_t Unptr>
constexpr std::size_t type_to_id_extension_apply(std::size_t ext) noexcept {
    constexpr std::size_t native_id = (Unptr & native_types_mask);
    constexpr std::size_t extensions = (Unptr & ~native_types_mask);
    static_assert(
        !((extensions >> bits_per_extension) & native_types_mask),
        "====================> Boost.PFR: Too many extensions for a single field (something close to `int************************** p;` is in the POD type)."
    );

    return (extensions >> bits_per_extension) | native_id | ext;
}

template <std::size_t Index>
using remove_1_ext = size_t_<
    ((Index & ~native_types_mask) << bits_per_extension) | (Index & native_types_mask)
>;

#ifdef _MSC_VER
#   pragma warning( pop )
#endif

///////////////////// Forward declarations

template <class Type> constexpr std::size_t type_to_id(identity<Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<const Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<const volatile Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<volatile Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type&>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_enum<Type>::value>* = 0) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_empty<Type>::value>* = 0) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_union<Type>::value>* = 0) noexcept;
template <class Type> constexpr size_array<sizeof(Type) * 3> type_to_id(identity<Type>, std::enable_if_t<!std::is_enum<Type>::value && !std::is_empty<Type>::value && !std::is_union<Type>::value>* = 0) noexcept;

template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_ptr_type> = 0) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ptr_type> = 0) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_volatile_ptr_type> = 0) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_volatile_ptr_type> = 0) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ref_type> = 0) noexcept;


///////////////////// Definitions of type_to_id and id_to_type for fundamental types
/// @cond
#define BOOST_MAGIC_GET_REGISTER_TYPE(Type, Index)              \
    constexpr std::size_t type_to_id(identity<Type>) noexcept { \
        return Index;                                           \
    }                                                           \
    constexpr Type id_to_type( size_t_<Index > ) noexcept {     \
        return detail::construct_helper<Type>();                \
    }                                                           \
    /**/
/// @endcond


// Register all base types here
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned char         , 1)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned short        , 2)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned int          , 3)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned long         , 4)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned long long    , 5)
BOOST_MAGIC_GET_REGISTER_TYPE(signed char           , 6)
BOOST_MAGIC_GET_REGISTER_TYPE(short                 , 7)
BOOST_MAGIC_GET_REGISTER_TYPE(int                   , 8)
BOOST_MAGIC_GET_REGISTER_TYPE(long                  , 9)
BOOST_MAGIC_GET_REGISTER_TYPE(long long             , 10)
BOOST_MAGIC_GET_REGISTER_TYPE(char                  , 11)
BOOST_MAGIC_GET_REGISTER_TYPE(wchar_t               , 12)
BOOST_MAGIC_GET_REGISTER_TYPE(char16_t              , 13)
BOOST_MAGIC_GET_REGISTER_TYPE(char32_t              , 14)
BOOST_MAGIC_GET_REGISTER_TYPE(float                 , 15)
BOOST_MAGIC_GET_REGISTER_TYPE(double                , 16)
BOOST_MAGIC_GET_REGISTER_TYPE(long double           , 17)
BOOST_MAGIC_GET_REGISTER_TYPE(bool                  , 18)
BOOST_MAGIC_GET_REGISTER_TYPE(void*                 , 19)
BOOST_MAGIC_GET_REGISTER_TYPE(const void*           , 20)
BOOST_MAGIC_GET_REGISTER_TYPE(volatile void*        , 21)
BOOST_MAGIC_GET_REGISTER_TYPE(const volatile void*  , 22)
BOOST_MAGIC_GET_REGISTER_TYPE(std::nullptr_t        , 23)
constexpr std::size_t tuple_begin_tag               = 24;
constexpr std::size_t tuple_end_tag                 = 25;

#undef BOOST_MAGIC_GET_REGISTER_TYPE

///////////////////// Definitions of type_to_id and id_to_type for types with extensions and nested types
template <class Type>
constexpr std::size_t type_to_id(identity<Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<const Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Const pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_const_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<const volatile Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Const volatile pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_const_volatile_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<volatile Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Volatile pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_volatile_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type&>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: References to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_ref_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_enum<Type>::value>*) noexcept {
    return typeid_conversions::type_to_id(identity<typename std::underlying_type<Type>::type >{});
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_empty<Type>::value>*) noexcept {
    static_assert(!std::is_empty<Type>::value, "====================> Boost.PFR: Empty classes/structures as members are not supported.");
    return 0;
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_union<Type>::value>*) noexcept {
    static_assert(
        !std::is_union<Type>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    return 0;
}

template <class Type>
constexpr size_array<sizeof(Type) * 3> type_to_id(identity<Type>, std::enable_if_t<!std::is_enum<Type>::value && !std::is_empty<Type>::value && !std::is_union<Type>::value>*) noexcept {
    constexpr auto t = detail::flat_array_of_type_ids<Type>();
    size_array<sizeof(Type) * 3> result {{tuple_begin_tag}};
    constexpr bool requires_tuplening = (
        (t.count_nonzeros() != 1)  || (t.count_nonzeros() == t.count_from_opening_till_matching_parenthis_seq(0, tuple_begin_tag, tuple_end_tag))
    );

    if (requires_tuplening) {
        for (std::size_t i = 0; i < t.size(); ++i)
            result.data[i + 1] = t.data[i];
        result.data[result.size() - 1] = tuple_end_tag;
    } else {
        for (std::size_t i = 0; i < t.size(); ++i)
            result.data[i] = t.data[i];
    }
    return result;
}



template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ptr_type>) noexcept {
    typedef decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}

template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_ptr_type>) noexcept {
    typedef const decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}

template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_volatile_ptr_type>) noexcept {
    typedef const volatile decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}


template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_volatile_ptr_type>) noexcept {
    typedef volatile decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}


template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ref_type>) noexcept {
    static_assert(!Index, "====================> Boost.PFR: References are not supported");
    return nullptr;
}

} // namespace typeid_conversions

///////////////////// Structure that remembers types as integers on a `constexpr operator Type()` call
struct ubiq_val {
    std::size_t* ref_;

    template <class T>
    constexpr void assign(const T& typeids) const noexcept {
        for (std::size_t i = 0; i < T::size(); ++i)
            ref_[i] = typeids.data[i];
    }

    constexpr void assign(std::size_t val) const noexcept {
        ref_[0] = val;
    }

    template <class Type>
    constexpr operator Type() const noexcept {
        constexpr auto typeids = typeid_conversions::type_to_id(identity<Type>{});
        assign(typeids);
        return detail::construct_helper<Type>();
    }
};

///////////////////// Structure that remembers size of the type on a `constexpr operator Type()` call
struct ubiq_sizes {
    std::size_t& ref_;

    template <class Type>
    constexpr operator Type() const noexcept {
        ref_ = sizeof(Type);
        return detail::construct_helper<Type>();
    }
};

///////////////////// Returns array of (offsets without accounting alignments). Required for keeping places for nested type ids
template <class T, std::size_t N, std::size_t... I>
constexpr size_array<N> get_type_offsets() noexcept {
    typedef size_array<N> array_t;
    array_t sizes{};
    T tmp{ ubiq_sizes{sizes.data[I]}... };
    (void)tmp;

    array_t offsets{{0}};
    for (std::size_t i = 1; i < N; ++i)
        offsets.data[i] = offsets.data[i - 1] + sizes.data[i - 1];

    return offsets;
}

///////////////////// Returns array of typeids and zeros if construtor of a type accepts sizeof...(I) parameters
template <class T, std::size_t N, std::size_t... I>
constexpr void* flat_type_to_array_of_type_ids(std::size_t* types, std::index_sequence<I...>) noexcept
{
    static_assert(
        N <= sizeof(T),
        "====================> Boost.PFR: Bit fields are not supported."
    );

    constexpr auto offsets = detail::get_type_offsets<T, N, I...>();
    T tmp{ ubiq_val{types + get<I>(offsets) * 3}... };
    (void)types;
    (void)tmp;
    (void)offsets; // If type is empty offsets are not used
    return nullptr;
}

///////////////////// Returns array of typeids and zeros
template <class T>
constexpr size_array<sizeof(T) * 3> fields_count_and_type_ids_with_zeros() noexcept {
    size_array<sizeof(T) * 3> types{};
    constexpr std::size_t N = detail::fields_count<T>();
    detail::flat_type_to_array_of_type_ids<T, N>(types.data, detail::make_index_sequence<N>());
    return types;
}

///////////////////// Returns array of typeids without zeros
template <class T>
constexpr auto flat_array_of_type_ids() noexcept {
    constexpr auto types = detail::fields_count_and_type_ids_with_zeros<T>();
    constexpr std::size_t count = types.count_nonzeros();
    size_array<count> res{};
    std::size_t j = 0;
    for (std::size_t i = 0; i < decltype(types)::size(); ++i) {
        if (types.data[i]) {
            res.data[j] = types.data[i];
            ++ j;
        }
    }

    return res;
}

///////////////////// Convert array of typeids into sequence_tuple::tuple

template <class T, std::size_t First, std::size_t... I>
constexpr auto as_flat_tuple_impl(std::index_sequence<First, I...>) noexcept;

template <class T>
constexpr sequence_tuple::tuple<> as_flat_tuple_impl(std::index_sequence<>) noexcept {
    return sequence_tuple::tuple<>{};
}

template <std::size_t Increment, std::size_t... I>
constexpr auto increment_index_sequence(std::index_sequence<I...>) noexcept {
    return std::index_sequence<I + Increment...>{};
}

template <class T, std::size_t V, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<V>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(SubtupleLength == 0, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    return typeid_conversions::id_to_type(size_t_<V>{});
}

template <class T, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<typeid_conversions::tuple_end_tag>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(sizeof(T) == 0, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    return int{};
}

template <class T, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<typeid_conversions::tuple_begin_tag>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(SubtupleLength > 2, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    constexpr auto seq = detail::make_index_sequence<SubtupleLength - 2>{};
    return detail::as_flat_tuple_impl<T>( detail::increment_index_sequence<I + 1>(seq) );
}


template <class Array>
constexpr Array remove_subtuples(Array indexes_plus_1, const Array& subtuple_lengths) noexcept {
    for (std::size_t i = 0; i < subtuple_lengths.size(); ++i) {
        if (subtuple_lengths.data[i]) {
            const std::size_t skips_count = subtuple_lengths.data[i];
            for (std::size_t j = i + 1; j < skips_count + i; ++j) {
                indexes_plus_1.data[j] = 0;
            }
            i += skips_count - 1;
        }
    }
    return indexes_plus_1;
}

template <std::size_t N, class Array>
constexpr size_array<N> resize_dropping_zeros_and_decrementing(size_t_<N>, const Array& a) noexcept {
    size_array<N> result{};
    std::size_t result_indx = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a.data[i]) {
            result.data[result_indx] = static_cast<std::size_t>(a.data[i] - 1);
            ++ result_indx;
        }
    }

    return result;
}

template <class T, std::size_t First, std::size_t... I, std::size_t... INew>
constexpr auto as_flat_tuple_impl_drop_helpers(std::index_sequence<First, I...>, std::index_sequence<INew...>) noexcept {
    constexpr auto a = detail::flat_array_of_type_ids<T>();

    constexpr size_array<sizeof...(I) + 1> subtuples_length {{
        a.count_from_opening_till_matching_parenthis_seq(First, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag),
        a.count_from_opening_till_matching_parenthis_seq(I, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag)...
    }};

    constexpr size_array<sizeof...(I) + 1> type_indexes_with_subtuple_internals {{ 1, 1 + I - First...}};
    constexpr auto type_indexes_plus_1_and_zeros_as_skips = detail::remove_subtuples(type_indexes_with_subtuple_internals, subtuples_length);
    constexpr auto new_size = size_t_<type_indexes_plus_1_and_zeros_as_skips.count_nonzeros()>{};
    constexpr auto type_indexes = detail::resize_dropping_zeros_and_decrementing(new_size, type_indexes_plus_1_and_zeros_as_skips);

    typedef sequence_tuple::tuple<
        decltype(detail::prepare_subtuples<T>(
            size_t_< a.data[ First + type_indexes.data[INew] ]          >{},    // id of type
            size_t_< First + type_indexes.data[INew]                    >{},    // index of current id in `a`
            size_t_< subtuples_length.data[ type_indexes.data[INew] ]   >{}     // if id of type is tuple, then length of that tuple
        ))...
    > subtuples_uncleanuped_t;

    return subtuples_uncleanuped_t{};
}

template <class Array>
constexpr std::size_t count_skips_in_array(std::size_t begin_index, std::size_t end_index, const Array& a) noexcept {
    std::size_t skips = 0;
    for (std::size_t i = begin_index; i < end_index; ++i) {
        if (a.data[i] == typeid_conversions::tuple_begin_tag) {
            const std::size_t this_tuple_size = a.count_from_opening_till_matching_parenthis_seq(i, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag) - 1;
            skips += this_tuple_size;
            i += this_tuple_size - 1;
        }
    }

    return skips;
}

template <class T, std::size_t First, std::size_t... I>
constexpr auto as_flat_tuple_impl(std::index_sequence<First, I...>) noexcept {
    constexpr auto a = detail::flat_array_of_type_ids<T>();
    constexpr std::size_t count_of_I = sizeof...(I);

    return detail::as_flat_tuple_impl_drop_helpers<T>(
        std::index_sequence<First, I...>{},
        detail::make_index_sequence< 1 + count_of_I - count_skips_in_array(First, First + count_of_I, a) >{}
    );
}

template <class T>
constexpr auto internal_tuple_with_same_alignment() noexcept {
    typedef typename std::remove_cv<T>::type type;

    static_assert(
        std::is_trivial<type>::value && std::is_standard_layout<type>::value,
        "====================> Boost.PFR: Type can not be reflected without Loophole or C++17, because it's not POD"
    );
    static_assert(!std::is_reference<type>::value, "====================> Boost.PFR: Not applyable");
    constexpr auto res = detail::as_flat_tuple_impl<type>(
        detail::make_index_sequence< decltype(detail::flat_array_of_type_ids<type>())::size() >()
    );

    return res;
}

template <class T>
using internal_tuple_with_same_alignment_t = decltype( detail::internal_tuple_with_same_alignment<T>() );


///////////////////// Flattening
struct ubiq_is_flat_refelectable {
    bool& is_flat_refelectable;

    template <class Type>
    constexpr operator Type() const noexcept {
        is_flat_refelectable = std::is_fundamental<std::remove_pointer_t<Type>>::value;
        return {};
    }
};

template <class T, std::size_t... I>
constexpr bool is_flat_refelectable(std::index_sequence<I...>) noexcept {
    constexpr std::size_t fields = sizeof...(I);
    bool result[fields] = {static_cast<bool>(I)...};
    const T v{ ubiq_is_flat_refelectable{result[I]}... };
    (void)v;

    for (std::size_t i = 0; i < fields; ++i) {
        if (!result[i]) {
            return false;
        }
    }

    return true;
}

template<class T>
constexpr bool is_flat_refelectable(std::index_sequence<>) noexcept {
    return true; ///< all empty structs always flat refelectable
}

template <class T>
auto tie_as_flat_tuple(T& lvalue) noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    using type = std::remove_cv_t<T>;
    using tuple_type = internal_tuple_with_same_alignment_t<type>;

    offset_based_getter<type, tuple_type> getter;
    return boost::pfr::detail::make_flat_tuple_of_references(lvalue, getter, size_t_<0>{}, size_t_<tuple_type::size_v>{});
}

template <class T>
auto tie_as_tuple(T& val) noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    static_assert(
        boost::pfr::detail::is_flat_refelectable<T>( detail::make_index_sequence<boost::pfr::detail::fields_count<T>()>{} ),
        "====================> Boost.PFR: Not possible in C++14 to represent that type without loosing information. Change type definition or enable C++17"
    );
    return boost::pfr::detail::tie_as_flat_tuple(val);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////// Structure that can be converted to copy of anything
struct ubiq_constructor_constexpr_copy {
    std::size_t ignore;

    template <class Type>
    constexpr operator Type() const noexcept {
        static_assert(
            std::is_trivially_destructible<Type>::value,
            "====================> Boost.PFR: One of the fields in the type passed to `for_each_field` has non trivial destructor."
        );
        return {};
    }
};

/////////////////////

template <class T, std::size_t... I>
struct is_constexpr_aggregate_initializable { // TODO: try to fix it
    template <T = T{ ubiq_constructor_constexpr_copy{I}... } >
    static std::true_type test(long) noexcept;

    static std::false_type test(...) noexcept;

    static constexpr decltype( test(0) ) value{};
};


template <class T, class F, std::size_t I0, std::size_t... I, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<I0, I...>, identity<Fields>...);

template <class T, class F, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<>, identity<Fields>...);

template <class T, class F, class IndexSeq, class... Fields>
struct next_step {
    T& t;
    F& f;

    template <class Field>
    operator Field() const {
         boost::pfr::detail::for_each_field_in_depth(
             t,
             std::forward<F>(f),
             IndexSeq{},
             identity<Fields>{}...,
             identity<Field>{}
         );

         return {};
    }
};

template <class T, class F, std::size_t I0, std::size_t... I, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<I0, I...>, identity<Fields>...) {
    (void)std::add_const_t<std::remove_reference_t<T>>{
        Fields{}...,
        next_step<T, F, std::index_sequence<I...>, Fields...>{t, f},
        ubiq_constructor_constexpr_copy{I}...
    };
}

template <class T, class F, class... Fields>
void for_each_field_in_depth(T& lvalue, F&& f, std::index_sequence<>, identity<Fields>...) {
    using tuple_type = sequence_tuple::tuple<Fields...>;

    offset_based_getter<std::remove_cv_t<std::remove_reference_t<T>>, tuple_type> getter;
    std::forward<F>(f)(
        boost::pfr::detail::make_flat_tuple_of_references(lvalue, getter, size_t_<0>{}, size_t_<sizeof...(Fields)>{})
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher_1(T& t, F&& f, std::index_sequence<I...>, std::true_type /*is_flat_refelectable*/) {
    std::forward<F>(f)(
        boost::pfr::detail::tie_as_flat_tuple(t)
    );
}


template <class T, class F, std::size_t... I>
void for_each_field_dispatcher_1(T& t, F&& f, std::index_sequence<I...>, std::false_type /*is_flat_refelectable*/) {
    boost::pfr::detail::for_each_field_in_depth(
        t,
        std::forward<F>(f),
        std::index_sequence<I...>{}
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher(T& t, F&& f, std::index_sequence<I...>) {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );

    /// Compile time error at this point means that you have called `for_each_field` or some other non-flat function or operator for a
    /// type that is not constexpr aggregate initializable.
    ///
    /// Make sure that all the fields of your type have constexpr default construtors and trivial destructors.
    /// Or compile in C++17 mode.
    constexpr T tmp{ ubiq_constructor_constexpr_copy{I}... };
    (void)tmp;

    //static_assert(is_constexpr_aggregate_initializable<T, I...>::value, "====================> Boost.PFR: T must be a constexpr initializable type");

    constexpr bool is_flat_refelectable_val = detail::is_flat_refelectable<T>( std::index_sequence<I...>{} );
    detail::for_each_field_dispatcher_1(
        t,
        std::forward<F>(f),
        std::index_sequence<I...>{},
        std::integral_constant<bool, is_flat_refelectable_val>{}
    );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __clang__
#   pragma clang diagnostic pop
#endif

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP
