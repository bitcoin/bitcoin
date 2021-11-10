//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CHANNEL_HPP
#define BOOST_GIL_CHANNEL_HPP

#include <boost/gil/utilities.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/config/pragma_message.hpp>
#include <boost/integer/integer_mask.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

#ifdef BOOST_GIL_DOXYGEN_ONLY
/// \def BOOST_GIL_CONFIG_HAS_UNALIGNED_ACCESS
/// \brief Define to allow unaligned memory access for models of packed channel value.
/// Theoretically (or historically?) on platforms which support dereferencing on
/// non-word memory boundary, unaligned access may result in performance improvement.
/// \warning Unfortunately, this optimization may be a C/C++ strict aliasing rules
/// violation, if accessed data buffer has effective type that cannot be aliased
/// without leading to undefined behaviour.
#define BOOST_GIL_CONFIG_HAS_UNALIGNED_ACCESS
#endif

#ifdef BOOST_GIL_CONFIG_HAS_UNALIGNED_ACCESS
#if defined(sun) || defined(__sun) || \             // SunOS
    defined(__osf__) || defined(__osf) || \         // Tru64
    defined(_hpux) || defined(hpux) || \            // HP-UX
    defined(__arm__) || defined(__ARM_ARCH) || \    // ARM
    defined(_AIX)                                   // AIX
#error Unaligned access strictly disabled for some UNIX platforms or ARM architecture
#elif defined(__i386__) || defined(__x86_64__) || defined(__vax__)
    // The check for little-endian architectures that tolerate unaligned memory
    // accesses is just an optimization. Nothing will break if it fails to detect
    // a suitable architecture.
    //
    // Unfortunately, this optimization may be a C/C++ strict aliasing rules violation
    // if accessed data buffer has effective type that cannot be aliased
    // without leading to undefined behaviour.
BOOST_PRAGMA_MESSAGE("CAUTION: Unaligned access tolerated on little-endian may cause undefined behaviour")
#else
#error Unaligned access disabled for unknown platforms and architectures
#endif
#endif // defined(BOOST_GIL_CONFIG_HAS_UNALIGNED_ACCESS)

namespace boost { namespace gil {

///////////////////////////////////////////
////  channel_traits
////
////  \ingroup ChannelModel
////  \class channel_traits
////  \brief defines properties of channels, such as their range and associated types
////
////  The channel traits must be defined for every model of ChannelConcept
////  Default traits are provided. For built-in types the default traits use
////  built-in pointer and reference and the channel range is the physical
////  range of the type. For classes, the default traits forward the associated types
////  and range to the class.
////
///////////////////////////////////////////

namespace detail {

template <typename T, bool IsClass>
struct channel_traits_impl;

// channel traits for custom class
template <typename T>
struct channel_traits_impl<T, true>
{
    using value_type = typename T::value_type;
    using reference = typename T::reference;
    using pointer = typename T::pointer;
    using const_reference = typename T::const_reference;
    using const_pointer = typename T::const_pointer;
    static constexpr bool is_mutable = T::is_mutable;
    static value_type min_value() { return T::min_value(); }
    static value_type max_value() { return T::max_value(); }
};

// channel traits implementation for built-in integral or floating point channel type
template <typename T>
struct channel_traits_impl<T, false>
{
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using const_reference = T const&;
    using const_pointer = T const*;
    static constexpr bool is_mutable = true;
    static value_type min_value() { return (std::numeric_limits<T>::min)(); }
    static value_type max_value() { return (std::numeric_limits<T>::max)(); }
};

// channel traits implementation for constant built-in scalar or floating point type
template <typename T>
struct channel_traits_impl<T const, false> : channel_traits_impl<T, false>
{
    using reference = T const&;
    using pointer = T const*;
    static constexpr bool is_mutable = false;
};

} // namespace detail

/**
\ingroup ChannelModel
\brief Traits for channels. Contains the following members:
\code
template <typename Channel>
struct channel_traits {
    using value_type = ...;
    using reference = ...;
    using pointer = ...;
    using const_reference = ...;
    using const_pointer = ...;

    static const bool is_mutable;
    static value_type min_value();
    static value_type max_value();
};
\endcode
*/
template <typename T>
struct channel_traits : detail::channel_traits_impl<T, std::is_class<T>::value> {};

// Channel traits for C++ reference type - remove the reference
template <typename T>
struct channel_traits<T&> : channel_traits<T> {};

// Channel traits for constant C++ reference type
template <typename T>
struct channel_traits<T const&> : channel_traits<T>
{
    using reference = typename channel_traits<T>::const_reference;
    using pointer = typename channel_traits<T>::const_pointer;
    static constexpr bool is_mutable = false;
};

///////////////////////////////////////////
////  scoped_channel_value
///////////////////////////////////////////

/// \defgroup ScopedChannelValue scoped_channel_value
/// \ingroup ChannelModel
/// \brief A channel adaptor that modifies the range of the source channel. Models: ChannelValueConcept
///
/// Example:
/// \code
/// // Create a double channel with range [-0.5 .. 0.5]
/// struct double_minus_half  { static double apply() { return -0.5; } };
/// struct double_plus_half   { static double apply() { return  0.5; } };
/// using bits64custom_t = scoped_channel_value<double, double_minus_half, double_plus_half>;
///
/// // channel_convert its maximum should map to the maximum
/// bits64custom_t x = channel_traits<bits64custom_t>::max_value();
/// assert(x == 0.5);
/// uint16_t y = channel_convert<uint16_t>(x);
/// assert(y == 65535);
/// \endcode

/// \ingroup ScopedChannelValue
/// \brief A channel adaptor that modifies the range of the source channel. Models: ChannelValueConcept
/// \tparam BaseChannelValue base channel (models ChannelValueConcept)
/// \tparam MinVal class with a static apply() function returning the minimum channel values
/// \tparam MaxVal class with a static apply() function returning the maximum channel values
template <typename BaseChannelValue, typename MinVal, typename MaxVal>
struct scoped_channel_value
{
    using value_type = scoped_channel_value<BaseChannelValue, MinVal, MaxVal>;
    using reference = value_type&;
    using pointer = value_type*;
    using const_reference = value_type const&;
    using const_pointer = value_type const*;
    static constexpr bool is_mutable = channel_traits<BaseChannelValue>::is_mutable;

    using base_channel_t = BaseChannelValue;

    static value_type min_value() { return MinVal::apply(); }
    static value_type max_value() { return MaxVal::apply(); }

    scoped_channel_value() = default;
    scoped_channel_value(scoped_channel_value const& other) : value_(other.value_) {}
    scoped_channel_value& operator=(scoped_channel_value const& other) = default;
    scoped_channel_value(BaseChannelValue value) : value_(value) {}
    scoped_channel_value& operator=(BaseChannelValue value)
    {
        value_ = value;
        return *this;
    }

    scoped_channel_value& operator++() { ++value_; return *this; }
    scoped_channel_value& operator--() { --value_; return *this; }

    scoped_channel_value operator++(int) { scoped_channel_value tmp=*this; this->operator++(); return tmp; }
    scoped_channel_value operator--(int) { scoped_channel_value tmp=*this; this->operator--(); return tmp; }

    template <typename Scalar2> scoped_channel_value& operator+=(Scalar2 v) { value_+=v; return *this; }
    template <typename Scalar2> scoped_channel_value& operator-=(Scalar2 v) { value_-=v; return *this; }
    template <typename Scalar2> scoped_channel_value& operator*=(Scalar2 v) { value_*=v; return *this; }
    template <typename Scalar2> scoped_channel_value& operator/=(Scalar2 v) { value_/=v; return *this; }

    operator BaseChannelValue() const { return value_; }
private:
    BaseChannelValue value_{};
};

template <typename T>
struct float_point_zero
{
    static constexpr T apply() { return 0.0f; }
};

template <typename T>
struct float_point_one
{
    static constexpr T apply() { return 1.0f; }
};

///////////////////////////////////////////
////  Support for sub-byte channels. These are integral channels whose value is contained in a range of bits inside an integral type
///////////////////////////////////////////

// It is necessary for packed channels to have their own value type. They cannot simply use an integral large enough to store the data. Here is why:
// - Any operation that requires returning the result by value will otherwise return the built-in integral type, which will have incorrect range
//   That means that after getting the value of the channel we cannot properly do channel_convert, channel_invert, etc.
// - Two channels are declared compatible if they have the same value type. That means that a packed channel is incorrectly declared compatible with an integral type
namespace detail {

// returns the smallest fast unsigned integral type that has at least NumBits bits
template <int NumBits>
struct min_fast_uint :
    std::conditional
    <
        NumBits <= 8,
        std::uint_least8_t,
        typename std::conditional
        <
            NumBits <= 16,
            std::uint_least16_t,
            typename std::conditional
            <
                NumBits <= 32,
                std::uint_least32_t,
                std::uintmax_t
            >::type
        >::type
    >
{};

template <int NumBits>
struct num_value_fn
    : std::conditional<NumBits < 32, std::uint32_t, std::uint64_t>
{};

template <int NumBits>
struct max_value_fn
    : std::conditional<NumBits <= 32, std::uint32_t, std::uint64_t>
{};

} // namespace detail

/// \defgroup PackedChannelValueModel packed_channel_value
/// \ingroup ChannelModel
/// \brief Represents the value of an unsigned integral channel operating over a bit range. Models: ChannelValueConcept
/// Example:
/// \code
/// // A 4-bit unsigned integral channel.
/// using bits4 = packed_channel_value<4>;
///
/// assert(channel_traits<bits4>::min_value()==0);
/// assert(channel_traits<bits4>::max_value()==15);
/// assert(sizeof(bits4)==1);
/// static_assert(gil::is_channel_integral<bits4>::value, "");
/// \endcode

/// \ingroup PackedChannelValueModel
/// \brief The value of a subbyte channel. Models: ChannelValueConcept
template <int NumBits>
class packed_channel_value
{
public:
    using integer_t = typename detail::min_fast_uint<NumBits>::type;

    using value_type = packed_channel_value<NumBits>;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    static constexpr bool is_mutable = true;

    static value_type min_value() { return 0; }
    static value_type max_value() { return low_bits_mask_t< NumBits >::sig_bits; }

    packed_channel_value() = default;
    packed_channel_value(integer_t v)
    {
        value_ = static_cast<integer_t>(v & low_bits_mask_t<NumBits>::sig_bits_fast);
    }

    template <typename Scalar>
    packed_channel_value(Scalar v)
    {
        value_ = packed_channel_value(static_cast<integer_t>(v));
    }

    static unsigned int num_bits() { return NumBits; }

    operator integer_t() const { return value_; }

private:
    integer_t value_{};
};

namespace detail {

template <std::size_t K>
struct static_copy_bytes
{
    void operator()(unsigned char const* from, unsigned char* to) const
    {
        *to = *from;
        static_copy_bytes<K - 1>()(++from, ++to);
    }
};

template <>
struct static_copy_bytes<0>
{
    void operator()(unsigned char const*, unsigned char*) const {}
};

template <typename Derived, typename BitField, int NumBits, bool IsMutable>
class packed_channel_reference_base
{
protected:
    using data_ptr_t = typename std::conditional<IsMutable, void*, void const*>::type;
public:
    data_ptr_t _data_ptr;   // void* pointer to the first byte of the bit range

    using value_type = packed_channel_value<NumBits>;
    using reference = const Derived;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    static constexpr int num_bits = NumBits;
    static constexpr bool is_mutable = IsMutable;

    static value_type min_value()       { return channel_traits<value_type>::min_value(); }
    static value_type max_value()       { return channel_traits<value_type>::max_value(); }

    using bitfield_t = BitField;
    using integer_t = typename value_type::integer_t;

    packed_channel_reference_base(data_ptr_t data_ptr) : _data_ptr(data_ptr) {}
    packed_channel_reference_base(const packed_channel_reference_base& ref) : _data_ptr(ref._data_ptr) {}
    const Derived& operator=(integer_t v) const { set(v); return derived(); }

    const Derived& operator++() const { set(get()+1); return derived(); }
    const Derived& operator--() const { set(get()-1); return derived(); }

    Derived operator++(int) const { Derived tmp=derived(); this->operator++(); return tmp; }
    Derived operator--(int) const { Derived tmp=derived(); this->operator--(); return tmp; }

    template <typename Scalar2> const Derived& operator+=(Scalar2 v) const { set( static_cast<integer_t>(  get() + v )); return derived(); }
    template <typename Scalar2> const Derived& operator-=(Scalar2 v) const { set( static_cast<integer_t>(  get() - v )); return derived(); }
    template <typename Scalar2> const Derived& operator*=(Scalar2 v) const { set( static_cast<integer_t>(  get() * v )); return derived(); }
    template <typename Scalar2> const Derived& operator/=(Scalar2 v) const { set( static_cast<integer_t>(  get() / v )); return derived(); }

    operator integer_t() const { return get(); }
    data_ptr_t operator &() const {return _data_ptr;}
protected:

    using num_value_t = typename detail::num_value_fn<NumBits>::type;
    using max_value_t = typename detail::max_value_fn<NumBits>::type;

    static const num_value_t num_values = static_cast< num_value_t >( 1 ) << NumBits ;
    static const max_value_t max_val    = static_cast< max_value_t >( num_values - 1 );

#if defined(BOOST_GIL_CONFIG_HAS_UNALIGNED_ACCESS)
    const bitfield_t& get_data()                      const { return *static_cast<const bitfield_t*>(_data_ptr); }
    void              set_data(const bitfield_t& val) const {        *static_cast<      bitfield_t*>(_data_ptr) = val; }
#else
    bitfield_t get_data() const {
        bitfield_t ret;
        static_copy_bytes<sizeof(bitfield_t) >()(gil_reinterpret_cast_c<const unsigned char*>(_data_ptr),gil_reinterpret_cast<unsigned char*>(&ret));
        return ret;
    }
    void set_data(const bitfield_t& val) const {
        static_copy_bytes<sizeof(bitfield_t) >()(gil_reinterpret_cast_c<const unsigned char*>(&val),gil_reinterpret_cast<unsigned char*>(_data_ptr));
    }
#endif

private:
    void set(integer_t value) const {     // can this be done faster??
        this->derived().set_unsafe(((value % num_values) + num_values) % num_values);
    }
    integer_t get() const { return derived().get(); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};
}   // namespace detail

/// \defgroup PackedChannelReferenceModel packed_channel_reference
/// \ingroup ChannelModel
/// \brief Represents a reference proxy to a channel operating over a bit range whose offset is fixed at compile time. Models ChannelConcept
/// Example:
/// \code
/// // Reference to a 2-bit channel starting at bit 1 (i.e. the second bit)
/// using bits2_1_ref_t = packed_channel_reference<uint16_t,1,2,true> const;
///
/// uint16_t data=0;
/// bits2_1_ref_t channel_ref(&data);
/// channel_ref = channel_traits<bits2_1_ref_t>::max_value();   // == 3
/// assert(data == 6);                                          // == 3<<1 == 6
/// \endcode

/// \tparam BitField A type that holds the bits of the pixel from which the channel is referenced. Typically an integral type, like std::uint16_t
/// \tparam Defines the sequence of bits in the data value that contain the channel
/// \tparam true if the reference is mutable
template <typename BitField, int FirstBit, int NumBits, bool IsMutable>
class packed_channel_reference;

/// \tparam A type that holds the bits of the pixel from which the channel is referenced. Typically an integral type, like std::uint16_t
/// \tparam Defines the sequence of bits in the data value that contain the channel
/// \tparam true if the reference is mutable
template <typename BitField, int NumBits, bool IsMutable>
class packed_dynamic_channel_reference;

/// \ingroup PackedChannelReferenceModel
/// \brief A constant subbyte channel reference whose bit offset is fixed at compile time. Models ChannelConcept
template <typename BitField, int FirstBit, int NumBits>
class packed_channel_reference<BitField, FirstBit, NumBits, false>
    : public detail::packed_channel_reference_base
        <
            packed_channel_reference<BitField, FirstBit, NumBits, false>,
            BitField,
            NumBits,
            false
        >
{
    using parent_t = detail::packed_channel_reference_base
        <
            packed_channel_reference<BitField, FirstBit, NumBits, false>,
            BitField,
            NumBits,
            false
        >;

    friend class packed_channel_reference<BitField, FirstBit, NumBits, true>;

    static const BitField channel_mask = static_cast<BitField>(parent_t::max_val) << FirstBit;

    void operator=(packed_channel_reference const&);
public:
    using const_reference = packed_channel_reference<BitField,FirstBit,NumBits,false> const;
    using mutable_reference = packed_channel_reference<BitField,FirstBit,NumBits,true> const;
    using integer_t = typename parent_t::integer_t;

    explicit packed_channel_reference(const void* data_ptr) : parent_t(data_ptr) {}
    packed_channel_reference(const packed_channel_reference& ref) : parent_t(ref._data_ptr) {}
    packed_channel_reference(const mutable_reference& ref) : parent_t(ref._data_ptr) {}

    unsigned first_bit() const { return FirstBit; }

    integer_t get() const { return integer_t((this->get_data()&channel_mask) >> FirstBit); }
};

/// \ingroup PackedChannelReferenceModel
/// \brief A mutable subbyte channel reference whose bit offset is fixed at compile time. Models ChannelConcept
template <typename BitField, int FirstBit, int NumBits>
class packed_channel_reference<BitField,FirstBit,NumBits,true>
   : public detail::packed_channel_reference_base<packed_channel_reference<BitField,FirstBit,NumBits,true>,BitField,NumBits,true>
{
    using parent_t = detail::packed_channel_reference_base<packed_channel_reference<BitField,FirstBit,NumBits,true>,BitField,NumBits,true>;
    friend class packed_channel_reference<BitField,FirstBit,NumBits,false>;

    static const BitField channel_mask = static_cast< BitField >( parent_t::max_val ) << FirstBit;

public:
    using const_reference = packed_channel_reference<BitField,FirstBit,NumBits,false> const;
    using mutable_reference = packed_channel_reference<BitField,FirstBit,NumBits,true> const;
    using integer_t = typename parent_t::integer_t;

    explicit packed_channel_reference(void* data_ptr) : parent_t(data_ptr) {}
    packed_channel_reference(const packed_channel_reference& ref) : parent_t(ref._data_ptr) {}

    packed_channel_reference const& operator=(integer_t value) const
    {
        BOOST_ASSERT(value <= parent_t::max_val);
        set_unsafe(value);
        return *this;
    }

    const packed_channel_reference& operator=(const mutable_reference& ref) const { set_from_reference(ref.get_data()); return *this; }
    const packed_channel_reference& operator=(const const_reference&   ref) const { set_from_reference(ref.get_data()); return *this; }

    template <bool Mutable1>
    const packed_channel_reference& operator=(const packed_dynamic_channel_reference<BitField,NumBits,Mutable1>& ref) const { set_unsafe(ref.get()); return *this; }

    unsigned first_bit() const { return FirstBit; }

    integer_t get()                  const { return integer_t((this->get_data()&channel_mask) >> FirstBit); }
    void set_unsafe(integer_t value) const { this->set_data((this->get_data() & ~channel_mask) | (( static_cast< BitField >( value )<<FirstBit))); }
private:
    void set_from_reference(const BitField& other_bits) const { this->set_data((this->get_data() & ~channel_mask) | (other_bits & channel_mask)); }
};

}}  // namespace boost::gil

namespace std {
// We are forced to define swap inside std namespace because on some platforms (Visual Studio 8) STL calls swap qualified.
// swap with 'left bias':
// - swap between proxy and anything
// - swap between value type and proxy
// - swap between proxy and proxy

/// \ingroup PackedChannelReferenceModel
/// \brief swap for packed_channel_reference
template <typename BF, int FB, int NB, bool M, typename R>
inline
void swap(boost::gil::packed_channel_reference<BF, FB, NB, M> const x, R& y)
{
    boost::gil::swap_proxy
    <
        typename boost::gil::packed_channel_reference<BF, FB, NB, M>::value_type
    >(x, y);
}


/// \ingroup PackedChannelReferenceModel
/// \brief swap for packed_channel_reference
template <typename BF, int FB, int NB, bool M>
inline
void swap(
    typename boost::gil::packed_channel_reference<BF, FB, NB, M>::value_type& x,
    boost::gil::packed_channel_reference<BF, FB, NB, M> const y)
{
    boost::gil::swap_proxy
    <
        typename boost::gil::packed_channel_reference<BF, FB, NB, M>::value_type
    >(x,y);
}

/// \ingroup PackedChannelReferenceModel
/// \brief swap for packed_channel_reference
template <typename BF, int FB, int NB, bool M> inline
void swap(
    boost::gil::packed_channel_reference<BF, FB, NB, M> const x,
    boost::gil::packed_channel_reference<BF, FB, NB, M> const y)
{
    boost::gil::swap_proxy
    <
        typename boost::gil::packed_channel_reference<BF, FB, NB, M>::value_type
    >(x,y);
}

}   // namespace std

namespace boost { namespace gil {

/// \defgroup PackedChannelDynamicReferenceModel packed_dynamic_channel_reference
/// \ingroup ChannelModel
/// \brief Represents a reference proxy to a channel operating over a bit range whose offset is specified at run time. Models ChannelConcept
///
/// Example:
/// \code
/// // Reference to a 2-bit channel whose offset is specified at construction time
/// using bits2_dynamic_ref_t = packed_dynamic_channel_reference<uint8_t,2,true> const;
///
/// uint16_t data=0;
/// bits2_dynamic_ref_t channel_ref(&data,1);
/// channel_ref = channel_traits<bits2_dynamic_ref_t>::max_value();     // == 3
/// assert(data == 6);                                                  // == (3<<1) == 6
/// \endcode

/// \brief Models a constant subbyte channel reference whose bit offset is a runtime parameter. Models ChannelConcept
///        Same as packed_channel_reference, except that the offset is a runtime parameter
/// \ingroup PackedChannelDynamicReferenceModel
template <typename BitField, int NumBits>
class packed_dynamic_channel_reference<BitField,NumBits,false>
   : public detail::packed_channel_reference_base<packed_dynamic_channel_reference<BitField,NumBits,false>,BitField,NumBits,false>
{
    using parent_t = detail::packed_channel_reference_base<packed_dynamic_channel_reference<BitField,NumBits,false>,BitField,NumBits,false>;
    friend class packed_dynamic_channel_reference<BitField,NumBits,true>;

    unsigned _first_bit;     // 0..7

    void operator=(const packed_dynamic_channel_reference&);
public:
    using const_reference = packed_dynamic_channel_reference<BitField,NumBits,false> const;
    using mutable_reference = packed_dynamic_channel_reference<BitField,NumBits,true> const;
    using integer_t = typename parent_t::integer_t;

    packed_dynamic_channel_reference(const void* data_ptr, unsigned first_bit) : parent_t(data_ptr), _first_bit(first_bit) {}
    packed_dynamic_channel_reference(const const_reference&   ref) : parent_t(ref._data_ptr), _first_bit(ref._first_bit) {}
    packed_dynamic_channel_reference(const mutable_reference& ref) : parent_t(ref._data_ptr), _first_bit(ref._first_bit) {}

    unsigned first_bit() const { return _first_bit; }

    integer_t get() const {
        const BitField channel_mask = static_cast< integer_t >( parent_t::max_val ) <<_first_bit;
        return static_cast< integer_t >(( this->get_data()&channel_mask ) >> _first_bit );
    }
};

/// \brief Models a mutable subbyte channel reference whose bit offset is a runtime parameter. Models ChannelConcept
///        Same as packed_channel_reference, except that the offset is a runtime parameter
/// \ingroup PackedChannelDynamicReferenceModel
template <typename BitField, int NumBits>
class packed_dynamic_channel_reference<BitField,NumBits,true>
   : public detail::packed_channel_reference_base<packed_dynamic_channel_reference<BitField,NumBits,true>,BitField,NumBits,true>
{
    using parent_t = detail::packed_channel_reference_base<packed_dynamic_channel_reference<BitField,NumBits,true>,BitField,NumBits,true>;
    friend class packed_dynamic_channel_reference<BitField,NumBits,false>;

    unsigned _first_bit;

public:
    using const_reference = packed_dynamic_channel_reference<BitField,NumBits,false> const;
    using mutable_reference = packed_dynamic_channel_reference<BitField,NumBits,true> const;
    using integer_t = typename parent_t::integer_t;

    packed_dynamic_channel_reference(void* data_ptr, unsigned first_bit) : parent_t(data_ptr), _first_bit(first_bit) {}
    packed_dynamic_channel_reference(const packed_dynamic_channel_reference& ref) : parent_t(ref._data_ptr), _first_bit(ref._first_bit) {}

    packed_dynamic_channel_reference const& operator=(integer_t value) const
    {
        BOOST_ASSERT(value <= parent_t::max_val);
        set_unsafe(value);
        return *this;
    }

    const packed_dynamic_channel_reference& operator=(const mutable_reference& ref) const {  set_unsafe(ref.get()); return *this; }
    const packed_dynamic_channel_reference& operator=(const const_reference&   ref) const {  set_unsafe(ref.get()); return *this; }

    template <typename BitField1, int FirstBit1, bool Mutable1>
    const packed_dynamic_channel_reference& operator=(const packed_channel_reference<BitField1, FirstBit1, NumBits, Mutable1>& ref) const
        {  set_unsafe(ref.get()); return *this; }

    unsigned first_bit() const { return _first_bit; }

    integer_t get() const {
        const BitField channel_mask = static_cast< integer_t >( parent_t::max_val ) << _first_bit;
        return static_cast< integer_t >(( this->get_data()&channel_mask ) >> _first_bit );
    }

    void set_unsafe(integer_t value) const {
        const BitField channel_mask = static_cast< integer_t >( parent_t::max_val ) << _first_bit;
        this->set_data((this->get_data() & ~channel_mask) | value<<_first_bit);
    }
};
} }  // namespace boost::gil

namespace std {
// We are forced to define swap inside std namespace because on some platforms (Visual Studio 8) STL calls swap qualified.
// swap with 'left bias':
// - swap between proxy and anything
// - swap between value type and proxy
// - swap between proxy and proxy


/// \ingroup PackedChannelDynamicReferenceModel
/// \brief swap for packed_dynamic_channel_reference
template <typename BF, int NB, bool M, typename R> inline
void swap(const boost::gil::packed_dynamic_channel_reference<BF,NB,M> x, R& y) {
    boost::gil::swap_proxy<typename boost::gil::packed_dynamic_channel_reference<BF,NB,M>::value_type>(x,y);
}


/// \ingroup PackedChannelDynamicReferenceModel
/// \brief swap for packed_dynamic_channel_reference
template <typename BF, int NB, bool M> inline
void swap(typename boost::gil::packed_dynamic_channel_reference<BF,NB,M>::value_type& x, const boost::gil::packed_dynamic_channel_reference<BF,NB,M> y) {
    boost::gil::swap_proxy<typename boost::gil::packed_dynamic_channel_reference<BF,NB,M>::value_type>(x,y);
}

/// \ingroup PackedChannelDynamicReferenceModel
/// \brief swap for packed_dynamic_channel_reference
template <typename BF, int NB, bool M> inline
void swap(const boost::gil::packed_dynamic_channel_reference<BF,NB,M> x, const boost::gil::packed_dynamic_channel_reference<BF,NB,M> y) {
    boost::gil::swap_proxy<typename boost::gil::packed_dynamic_channel_reference<BF,NB,M>::value_type>(x,y);
}
}   // namespace std

// \brief Determines the fundamental type which may be used, e.g., to cast from larger to smaller channel types.
namespace boost { namespace gil {
template <typename T>
struct base_channel_type_impl { using type = T; };

template <int N>
struct base_channel_type_impl<packed_channel_value<N> >
{ using type = typename packed_channel_value<N>::integer_t; };

template <typename B, int F, int N, bool M>
struct base_channel_type_impl<packed_channel_reference<B, F, N, M> >
{
    using type = typename packed_channel_reference<B,F,N,M>::integer_t;
};

template <typename B, int N, bool M>
struct base_channel_type_impl<packed_dynamic_channel_reference<B, N, M> >
{
    using type = typename packed_dynamic_channel_reference<B,N,M>::integer_t;
};

template <typename ChannelValue, typename MinV, typename MaxV>
struct base_channel_type_impl<scoped_channel_value<ChannelValue, MinV, MaxV> >
{ using type = ChannelValue; };

template <typename T>
struct base_channel_type : base_channel_type_impl<typename std::remove_cv<T>::type> {};

}} //namespace boost::gil

#endif
