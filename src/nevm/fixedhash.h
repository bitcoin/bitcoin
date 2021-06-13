/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file FixedHash.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * The FixedHash fixed-size "hash" container type.
 */

#ifndef SYSCOIN_NEVM_FIXEDHASH_H
#define SYSCOIN_NEVM_FIXEDHASH_H

#include <array>
#include <cstdint>
#include <algorithm>
#include <random>
#include <boost/functional/hash.hpp>
#include <nevm/commondata.h>

namespace dev
{

/// Compile-time calculation of Log2 of constant values.
template <unsigned N> struct StaticLog2 { enum { result = 1 + StaticLog2<N/2>::result }; };
template <> struct StaticLog2<1> { enum { result = 0 }; };

extern std::random_device s_fixedHashEngine;

/// Fixed-size raw-_byte array container type, with an API optimised for storing hashes.
/// Transparently converts to/from the corresponding arithmetic type; this will
/// assume the data contained in the hash is big-endian.
template <unsigned N>
class FixedHash
{
public:
	/// The corresponding arithmetic type.
	using Arith = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

	/// The size of the container.
	enum { size = N };

	/// A dummy flag to avoid accidental construction from pointer.
	enum ConstructFromPointerType { ConstructFromPointer };

	/// Method to convert from a string.
	enum ConstructFromStringType { FromHex, FromBinary };

	/// Method to convert from a string.
	enum ConstructFromHashType { AlignLeft, AlignRight, FailIfDifferent };

	/// Construct an empty hash.
	FixedHash() { m_data.fill(0); }

	/// Construct from another hash, filling with zeroes or cropping as necessary.
	template <unsigned M> explicit FixedHash(FixedHash<M> const& _h, ConstructFromHashType _t = AlignLeft) { m_data.fill(0); unsigned c = std::min(M, N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _h[_t == AlignRight ? M - 1 - i : i]; }

	/// Convert from the corresponding arithmetic type.
	explicit FixedHash(Arith const& _arith) { toBigEndian(_arith, m_data); }

	/// Convert from unsigned
	explicit FixedHash(unsigned _u) { toBigEndian(_u, m_data); }

	/// Explicitly construct, copying from a _byte array.
	explicit FixedHash(bytes const& _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<unsigned>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<unsigned>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

	/// Explicitly construct, copying from a _byte array.
	explicit FixedHash(bytesConstRef _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<unsigned>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<unsigned>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

	/// Explicitly construct, copying from a bytes in memory with given pointer.
	explicit FixedHash(_byte const* _bs, ConstructFromPointerType) { memcpy(m_data.data(), _bs, N); }

	/// Explicitly construct, copying from a  string.
	explicit FixedHash(std::string const& _s, ConstructFromStringType _t = FromHex, ConstructFromHashType _ht = FailIfDifferent): FixedHash(_t == FromHex ? fromHex(_s, WhenError::Throw) : dev::asBytes(_s), _ht) {}

	/// Convert to arithmetic type.
	operator Arith() const { return fromBigEndian<Arith>(m_data); }

	/// @returns true iff this is the empty hash.
	explicit operator bool() const { return std::any_of(m_data.begin(), m_data.end(), [](_byte _b) { return _b != 0; }); }

	// The obvious comparison operators.
	bool operator==(FixedHash const& _c) const { return m_data == _c.m_data; }
	bool operator!=(FixedHash const& _c) const { return m_data != _c.m_data; }
	bool operator<(FixedHash const& _c) const { for (unsigned i = 0; i < N; ++i) if (m_data[i] < _c.m_data[i]) return true; else if (m_data[i] > _c.m_data[i]) return false; return false; }
	bool operator>=(FixedHash const& _c) const { return !operator<(_c); }
	bool operator<=(FixedHash const& _c) const { return operator==(_c) || operator<(_c); }
	bool operator>(FixedHash const& _c) const { return !operator<=(_c); }

	// The obvious binary operators.
	FixedHash& operator^=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] ^= _c.m_data[i]; return *this; }
	FixedHash operator^(FixedHash const& _c) const { return FixedHash(*this) ^= _c; }
	FixedHash& operator|=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] |= _c.m_data[i]; return *this; }
	FixedHash operator|(FixedHash const& _c) const { return FixedHash(*this) |= _c; }
	FixedHash& operator&=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] &= _c.m_data[i]; return *this; }
	FixedHash operator&(FixedHash const& _c) const { return FixedHash(*this) &= _c; }
	FixedHash operator~() const { FixedHash ret; for (unsigned i = 0; i < N; ++i) ret[i] = ~m_data[i]; return ret; }

	// Big-endian increment.
	FixedHash& operator++() { for (unsigned i = size; i > 0 && !++m_data[--i]; ) {} return *this; }

	/// @returns true if all one-bits in @a _c are set in this object.
	bool contains(FixedHash const& _c) const { return (*this & _c) == _c; }

	/// @returns a particular _byte from the hash.
	_byte& operator[](unsigned _i) { return m_data[_i]; }
	/// @returns a particular _byte from the hash.
	_byte operator[](unsigned _i) const { return m_data[_i]; }

	/// @returns an abridged version of the hash as a user-readable hex string.
	std::string abridged() const { return toHex(ref().cropped(0, 4)) + "\342\200\246"; }

	/// @returns a version of the hash as a user-readable hex string that leaves out the middle part.
	std::string abridgedMiddle() const { return toHex(ref().cropped(0, 4)) + "\342\200\246" + toHex(ref().cropped(N - 4)); }

	/// @returns the hash as a user-readable hex string.
	std::string hex() const { return toHex(ref()); }

	/// @returns a mutable _byte vector_ref to the object's data.
	bytesRef ref() { return bytesRef(m_data.data(), N); }

	/// @returns a constant _byte vector_ref to the object's data.
	bytesConstRef ref() const { return bytesConstRef(m_data.data(), N); }

	/// @returns a mutable _byte pointer to the object's data.
	_byte* data() { return m_data.data(); }

	/// @returns a constant _byte pointer to the object's data.
	_byte const* data() const { return m_data.data(); }

	/// @returns begin iterator.
	auto begin() const -> typename std::array<_byte, N>::const_iterator { return m_data.begin(); }

	/// @returns end iterator.
	auto end() const -> typename std::array<_byte, N>::const_iterator { return m_data.end(); }

	/// @returns a copy of the object's data as a _byte vector.
	bytes asBytes() const { return bytes(data(), data() + N); }

	/// @returns a mutable reference to the object's data as an STL array.
	std::array<_byte, N>& asArray() { return m_data; }

	/// @returns a constant reference to the object's data as an STL array.
	std::array<_byte, N> const& asArray() const { return m_data; }

	/// Populate with random data.
	template <class Engine>
	void randomize(Engine& _eng)
	{
		for (auto& i: m_data)
			i = (uint8_t)std::uniform_int_distribution<uint16_t>(0, 255)(_eng);
	}

	/// @returns a random valued object.
	static FixedHash random() { FixedHash ret; ret.randomize(s_fixedHashEngine); return ret; }

	struct hash
	{
		/// Make a hash of the object's data.
		size_t operator()(FixedHash const& _value) const { return boost::hash_range(_value.m_data.cbegin(), _value.m_data.cend()); }
	};

	template <unsigned P, unsigned M> inline FixedHash& shiftBloom(FixedHash<M> const& _h)
	{
		return (*this |= _h.template bloomPart<P, N>());
	}

	template <unsigned P, unsigned M> inline bool containsBloom(FixedHash<M> const& _h)
	{
		return contains(_h.template bloomPart<P, N>());
	}

	template <unsigned P, unsigned M> inline FixedHash<M> bloomPart() const
	{
		unsigned const c_bloomBits = M * 8;
		unsigned const c_mask = c_bloomBits - 1;
		unsigned const c_bloomBytes = (StaticLog2<c_bloomBits>::result + 7) / 8;

		static_assert((M & (M - 1)) == 0, "M must be power-of-two");
		static_assert(P * c_bloomBytes <= N, "out of range");

		FixedHash<M> ret;
		_byte const* p = data();
		for (unsigned i = 0; i < P; ++i)
		{
			unsigned index = 0;
			for (unsigned j = 0; j < c_bloomBytes; ++j, ++p)
				index = (index << 8) | *p;
			index &= c_mask;
			ret[M - 1 - index / 8] |= (1 << (index % 8));
		}
		return ret;
	}

	/// Returns the index of the first bit set to one, or size() * 8 if no bits are set.
	inline unsigned firstBitSet() const
	{
		unsigned ret = 0;
		for (auto d: m_data)
			if (d)
				for (;; ++ret, d <<= 1)
					if (d & 0x80)
						return ret;
					else {}
			else
				ret += 8;
		return ret;
	}

	void clear() { m_data.fill(0); }

private:
	std::array<_byte, N> m_data;		///< The binary data.
};

template <unsigned T>
class SecureFixedHash: private FixedHash<T>
{
public:
	using ConstructFromHashType = typename FixedHash<T>::ConstructFromHashType;
	using ConstructFromStringType = typename FixedHash<T>::ConstructFromStringType;
	using ConstructFromPointerType = typename FixedHash<T>::ConstructFromPointerType;
	SecureFixedHash() = default;
	explicit SecureFixedHash(bytes const& _b, ConstructFromHashType _t = FixedHash<T>::FailIfDifferent): FixedHash<T>(_b, _t) {}
	explicit SecureFixedHash(bytesConstRef _b, ConstructFromHashType _t = FixedHash<T>::FailIfDifferent): FixedHash<T>(_b, _t) {}
	explicit SecureFixedHash(bytesSec const& _b, ConstructFromHashType _t = FixedHash<T>::FailIfDifferent): FixedHash<T>(_b.ref(), _t) {}
	template <unsigned M> explicit SecureFixedHash(FixedHash<M> const& _h, ConstructFromHashType _t = FixedHash<T>::AlignLeft): FixedHash<T>(_h, _t) {}
	template <unsigned M> explicit SecureFixedHash(SecureFixedHash<M> const& _h, ConstructFromHashType _t = FixedHash<T>::AlignLeft): FixedHash<T>(_h.makeInsecure(), _t) {}
	explicit SecureFixedHash(std::string const& _s, ConstructFromStringType _t = FixedHash<T>::FromHex, ConstructFromHashType _ht = FixedHash<T>::FailIfDifferent): FixedHash<T>(_s, _t, _ht) {}
	explicit SecureFixedHash(bytes const* _d, ConstructFromPointerType _t): FixedHash<T>(_d, _t) {}
	~SecureFixedHash() { ref().cleanse(); }

	SecureFixedHash<T>& operator=(SecureFixedHash<T> const& _c)
	{
		if (&_c == this)
			return *this;
		ref().cleanse();
		FixedHash<T>::operator=(static_cast<FixedHash<T> const&>(_c));
		return *this;
	}

	using FixedHash<T>::size;

	bytesSec asBytesSec() const { return bytesSec(ref()); }

	FixedHash<T> const& makeInsecure() const { return static_cast<FixedHash<T> const&>(*this); }
	FixedHash<T>& writable() { clear(); return static_cast<FixedHash<T>&>(*this); }

	using FixedHash<T>::operator bool;

	// The obvious comparison operators.
	bool operator==(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator==(static_cast<FixedHash<T> const&>(_c)); }
	bool operator!=(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator!=(static_cast<FixedHash<T> const&>(_c)); }
	bool operator<(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator<(static_cast<FixedHash<T> const&>(_c)); }
	bool operator>=(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator>=(static_cast<FixedHash<T> const&>(_c)); }
	bool operator<=(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator<=(static_cast<FixedHash<T> const&>(_c)); }
	bool operator>(SecureFixedHash const& _c) const { return static_cast<FixedHash<T> const&>(*this).operator>(static_cast<FixedHash<T> const&>(_c)); }

	using FixedHash<T>::operator==;
	using FixedHash<T>::operator!=;
	using FixedHash<T>::operator<;
	using FixedHash<T>::operator>=;
	using FixedHash<T>::operator<=;
	using FixedHash<T>::operator>;

	// The obvious binary operators.
	SecureFixedHash& operator^=(FixedHash<T> const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(_c); return *this; }
	SecureFixedHash operator^(FixedHash<T> const& _c) const { return SecureFixedHash(*this) ^= _c; }
	SecureFixedHash& operator|=(FixedHash<T> const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(_c); return *this; }
	SecureFixedHash operator|(FixedHash<T> const& _c) const { return SecureFixedHash(*this) |= _c; }
	SecureFixedHash& operator&=(FixedHash<T> const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(_c); return *this; }
	SecureFixedHash operator&(FixedHash<T> const& _c) const { return SecureFixedHash(*this) &= _c; }

	SecureFixedHash& operator^=(SecureFixedHash const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(static_cast<FixedHash<T> const&>(_c)); return *this; }
	SecureFixedHash operator^(SecureFixedHash const& _c) const { return SecureFixedHash(*this) ^= _c; }
	SecureFixedHash& operator|=(SecureFixedHash const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(static_cast<FixedHash<T> const&>(_c)); return *this; }
	SecureFixedHash operator|(SecureFixedHash const& _c) const { return SecureFixedHash(*this) |= _c; }
	SecureFixedHash& operator&=(SecureFixedHash const& _c) { static_cast<FixedHash<T>&>(*this).operator^=(static_cast<FixedHash<T> const&>(_c)); return *this; }
	SecureFixedHash operator&(SecureFixedHash const& _c) const { return SecureFixedHash(*this) &= _c; }
	SecureFixedHash operator~() const { auto r = ~static_cast<FixedHash<T> const&>(*this); return static_cast<SecureFixedHash const&>(r); }

	using FixedHash<T>::abridged;
	using FixedHash<T>::abridgedMiddle;

	bytesConstRef ref() const { return FixedHash<T>::ref(); }
	_byte const* data() const { return FixedHash<T>::data(); }

	static SecureFixedHash<T> random() { SecureFixedHash<T> ret; ret.randomize(s_fixedHashEngine); return ret; }
	using FixedHash<T>::firstBitSet;

	void clear() { ref().cleanse(); }
};

/// Fast equality operator for h256.
template<> inline bool FixedHash<32>::operator==(FixedHash<32> const& _other) const
{
	const uint64_t* hash1 = (const uint64_t*)data();
	const uint64_t* hash2 = (const uint64_t*)_other.data();
	return (hash1[0] == hash2[0]) && (hash1[1] == hash2[1]) && (hash1[2] == hash2[2]) && (hash1[3] == hash2[3]);
}

/// Fast std::hash compatible hash function object for h256.
template<> inline size_t FixedHash<32>::hash::operator()(FixedHash<32> const& value) const
{
	uint64_t const* data = reinterpret_cast<uint64_t const*>(value.data());
	return boost::hash_range(data, data + 4);
}

/// Stream I/O for the FixedHash class.
template <unsigned N>
inline std::ostream& operator<<(std::ostream& _out, FixedHash<N> const& _h)
{
	_out << toHex(_h);
	return _out;
}

/// Stream I/O for the SecureFixedHash class.
template <unsigned N>
inline std::ostream& operator<<(std::ostream& _out, SecureFixedHash<N> const& _h)
{
	_out << "SecureFixedHash#" << std::hex << typename FixedHash<N>::hash()(_h.makeInsecure()) << std::dec;
	return _out;
}

// Common types of FixedHash.
using h2048 = FixedHash<256>;
using h1024 = FixedHash<128>;
using h520 = FixedHash<65>;
using h512 = FixedHash<64>;
using h256 = FixedHash<32>;
using h160 = FixedHash<20>;
using h128 = FixedHash<16>;
using h64 = FixedHash<8>;
using h512s = std::vector<h512>;
using h256s = std::vector<h256>;
using h160s = std::vector<h160>;
using h256Set = std::set<h256>;
using h160Set = std::set<h160>;
using h256Hash = std::unordered_set<h256>;
using h160Hash = std::unordered_set<h160>;

/// Convert the given value into h160 (160-bit unsigned integer) using the right 20 bytes.
inline h160 right160(h256 const& _t)
{
	h160 ret;
	memcpy(ret.data(), _t.data() + 12, 20);
	return ret;
}

h128 fromUUID(std::string const& _uuid);

std::string toUUID(h128 const& _uuid);

inline std::string toString(h256s const& _bs)
{
	std::ostringstream out;
	out << "[ ";
	for (h256 const& i: _bs)
		out << i.abridged() << ", ";
	out << "]";
	return out.str();
}

}

namespace std
{
	/// Forward std::hash<dev::FixedHash> to dev::FixedHash::hash.
	template<> struct hash<dev::h64>: dev::h64::hash {};
	template<> struct hash<dev::h128>: dev::h128::hash {};
	template<> struct hash<dev::h160>: dev::h160::hash {};
	template<> struct hash<dev::h256>: dev::h256::hash {};
	template<> struct hash<dev::h512>: dev::h512::hash {};
}
  #endif // SYSCOIN_NEVM_FIXEDHASH_H