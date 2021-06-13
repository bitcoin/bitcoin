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
/** @file CommonData.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * Shared algorithms and data types.
 */
#ifndef SYSCOIN_NEVM_COMMONDATA_H
#define SYSCOIN_NEVM_COMMONDATA_H

#include <vector>
#include <algorithm>
#include <unordered_set>
#include <type_traits>
#include <cstring>
#include <string>
#include <nevm/common.h>

namespace dev
{

// String conversion functions, mainly to/from hex/nibble/_byte representations.

enum class WhenError
{
	DontThrow = 0,
	Throw = 1,
};

template <class Iterator>
std::string toHex(Iterator _it, Iterator _end, std::string const& _prefix)
{
	typedef std::iterator_traits<Iterator> traits;
	static_assert(sizeof(typename traits::value_type) == 1, "toHex needs _byte-sized element type");

	static char const* hexdigits = "0123456789abcdef";
	size_t off = _prefix.size();
	std::string hex(std::distance(_it, _end)*2 + off, '0');
	hex.replace(0, off, _prefix);
	for (; _it != _end; _it++)
	{
		hex[off++] = hexdigits[(*_it >> 4) & 0x0f];
		hex[off++] = hexdigits[*_it & 0x0f];
	}
	return hex;
}

/// Convert a series of bytes to the corresponding hex string.
/// @example toHex("A\x69") == "4169"
template <class T> std::string toHex(T const& _data)
{
	return toHex(_data.begin(), _data.end(), "");
}

/// Convert a series of bytes to the corresponding hex string with 0x prefix.
/// @example toHexPrefixed("A\x69") == "0x4169"
template <class T> std::string toHexPrefixed(T const& _data)
{
	return toHex(_data.begin(), _data.end(), "0x");
}

/// Converts a (printable) ASCII hex string into the corresponding _byte stream.
/// @example fromHex("41626261") == asBytes("Abba")
/// If _throw = ThrowType::DontThrow, it replaces bad hex characters with 0's, otherwise it will throw an exception.
bytes fromHex(std::string const& _s, WhenError _throw = WhenError::DontThrow);

/// @returns true if @a _s is a hex string.
bool isHex(std::string const& _s) noexcept;

/// @returns true if @a _hash is a hash conforming to FixedHash type @a T.
template <class T> static bool isHash(std::string const& _hash)
{
	return (_hash.size() == T::size * 2 || (_hash.size() == T::size * 2 + 2 && _hash.substr(0, 2) == "0x")) && isHex(_hash);
}

/// Converts _byte array to a string containing the same (binary) data. Unless
/// the _byte array happens to contain ASCII data, this won't be printable.
inline std::string asString(bytes const& _b)
{
	return std::string((char const*)_b.data(), (char const*)(_b.data() + _b.size()));
}

/// Converts _byte array ref to a string containing the same (binary) data. Unless
/// the _byte array happens to contain ASCII data, this won't be printable.
inline std::string asString(bytesConstRef _b)
{
	return std::string((char const*)_b.data(), (char const*)(_b.data() + _b.size()));
}

/// Converts a string to a _byte array containing the string's (_byte) data.
inline bytes asBytes(std::string const& _b)
{
	return bytes((_byte const*)_b.data(), (_byte const*)(_b.data() + _b.size()));
}

/// Converts a string into the big-endian base-16 stream of integers (NOT ASCII).
/// @example asNibbles("A")[0] == 4 && asNibbles("A")[1] == 1
bytes asNibbles(bytesConstRef const& _s);


// Big-endian to/from host endian conversion functions.

/// Converts a templated integer value to the big-endian _byte-stream represented on a templated collection.
/// The size of the collection object will be unchanged. If it is too small, it will not represent the
/// value properly, if too big then the additional elements will be zeroed out.
/// @a Out will typically be either std::string or bytes.
/// @a T will typically by unsigned, u160, u256 or bigint.
template <class T, class Out>
inline void toBigEndian(T _val, Out& o_out)
{
	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
	for (auto i = o_out.size(); i != 0; _val >>= 8, i--)
	{
		T v = _val & (T)0xff;
		o_out[i - 1] = (typename Out::value_type)(uint8_t)v;
	}
}

/// Converts a big-endian _byte-stream represented on a templated collection to a templated integer value.
/// @a _In will typically be either std::string or bytes.
/// @a T will typically by unsigned, u160, u256 or bigint.
template <class T, class _In>
inline T fromBigEndian(_In const& _bytes)
{
	T ret = (T)0;
	for (auto i: _bytes)
		ret = (T)((ret << 8) | (_byte)(typename std::make_unsigned<decltype(i)>::type)i);
	return ret;
}

/// Convenience functions for toBigEndian
inline std::string toBigEndianString(u256 _val) { std::string ret(32, '\0'); toBigEndian(_val, ret); return ret; }
inline std::string toBigEndianString(u160 _val) { std::string ret(20, '\0'); toBigEndian(_val, ret); return ret; }
inline bytes toBigEndian(u256 _val) { bytes ret(32); toBigEndian(_val, ret); return ret; }
inline bytes toBigEndian(u160 _val) { bytes ret(20); toBigEndian(_val, ret); return ret; }

/// Convenience function for toBigEndian.
/// @returns a _byte array just big enough to represent @a _val.
template <class T>
inline bytes toCompactBigEndian(T _val, unsigned _min = 0)
{
	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
	int i = 0;
	for (T v = _val; v; ++i, v >>= 8) {}
	bytes ret(std::max<unsigned>(_min, i), 0);
	toBigEndian(_val, ret);
	return ret;
}
inline bytes toCompactBigEndian(_byte _val, unsigned _min = 0)
{
	return (_min || _val) ? bytes{ _val } : bytes{};
}

/// Convenience function for toBigEndian.
/// @returns a string just big enough to represent @a _val.
template <class T>
inline std::string toCompactBigEndianString(T _val, unsigned _min = 0)
{
	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
	int i = 0;
	for (T v = _val; v; ++i, v >>= 8) {}
	std::string ret(std::max<unsigned>(_min, i), '\0');
	toBigEndian(_val, ret);
	return ret;
}

inline std::string toCompactHex(u256 _val, unsigned _min = 0)
{
	return toHex(toCompactBigEndian(_val, _min));
}

inline std::string toCompactHexPrefixed(u256 _val, unsigned _min = 0)
{
	return toHexPrefixed(toCompactBigEndian(_val, _min));
}

// Algorithms for string and string-like collections.

/// Escapes a string into the C-string representation.
/// @p _all if true will escape all characters, not just the unprintable ones.
std::string escaped(std::string const& _s, bool _all = true);

/// Determines the length of the common prefix of the two collections given.
/// @returns the number of elements both @a _t and @a _u share, in order, at the beginning.
/// @example commonPrefix("Hello world!", "Hello, world!") == 5
template <class T, class _U>
unsigned commonPrefix(T const& _t, _U const& _u)
{
	unsigned s = std::min<unsigned>(_t.size(), _u.size());
	for (unsigned i = 0;; ++i)
		if (i == s || _t[i] != _u[i])
			return i;
	return s;
}

/// Creates a random, printable, word.
std::string randomWord();

/// Determine bytes required to encode the given integer value. @returns 0 if @a _i is zero.
template <class T>
inline unsigned bytesRequired(T _i)
{
	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
	unsigned i = 0;
	for (; _i != 0; ++i, _i >>= 8) {}
	return i;
}

/// Trims a given number of elements from the front of a collection.
/// Only works for POD element types.
template <class T>
void trimFront(T& _t, unsigned _elements)
{
	static_assert(std::is_pod<typename T::value_type>::value, "");
	memmove(_t.data(), _t.data() + _elements, (_t.size() - _elements) * sizeof(_t[0]));
	_t.resize(_t.size() - _elements);
}

/// Pushes an element on to the front of a collection.
/// Only works for POD element types.
template <class T, class _U>
void pushFront(T& _t, _U _e)
{
	static_assert(std::is_pod<typename T::value_type>::value, "");
	_t.push_back(_e);
	memmove(_t.data() + 1, _t.data(), (_t.size() - 1) * sizeof(_e));
	_t[0] = _e;
}

/// Concatenate two vectors of elements of POD types.
template <class T>
inline std::vector<T>& operator+=(std::vector<typename std::enable_if<std::is_pod<T>::value, T>::type>& _a, std::vector<T> const& _b)
{
	auto s = _a.size();
	_a.resize(_a.size() + _b.size());
	memcpy(_a.data() + s, _b.data(), _b.size() * sizeof(T));
	return _a;

}

/// Concatenate two vectors of elements.
template <class T>
inline std::vector<T>& operator+=(std::vector<typename std::enable_if<!std::is_pod<T>::value, T>::type>& _a, std::vector<T> const& _b)
{
	_a.reserve(_a.size() + _b.size());
	for (auto& i: _b)
		_a.push_back(i);
	return _a;
}

/// Insert the contents of a container into a set
template <class T, class U> std::set<T>& operator+=(std::set<T>& _a, U const& _b)
{
	for (auto const& i: _b)
		_a.insert(i);
	return _a;
}

/// Insert the contents of a container into an unordered_set
template <class T, class U> std::unordered_set<T>& operator+=(std::unordered_set<T>& _a, U const& _b)
{
	for (auto const& i: _b)
		_a.insert(i);
	return _a;
}

/// Concatenate the contents of a container onto a vector
template <class T, class U> std::vector<T>& operator+=(std::vector<T>& _a, U const& _b)
{
	for (auto const& i: _b)
		_a.push_back(i);
	return _a;
}

/// Insert the contents of a container into a set
template <class T, class U> std::set<T> operator+(std::set<T> _a, U const& _b)
{
	return _a += _b;
}

/// Insert the contents of a container into an unordered_set
template <class T, class U> std::unordered_set<T> operator+(std::unordered_set<T> _a, U const& _b)
{
	return _a += _b;
}

/// Concatenate the contents of a container onto a vector
template <class T, class U> std::vector<T> operator+(std::vector<T> _a, U const& _b)
{
	return _a += _b;
}

/// Concatenate two vectors of elements.
template <class T>
inline std::vector<T> operator+(std::vector<T> const& _a, std::vector<T> const& _b)
{
	std::vector<T> ret(_a);
	return ret += _b;
}


/// Make normal string from fixed-length string.
std::string toString(string32 const& _s);

template<class T, class U>
std::vector<T> keysOf(std::map<T, U> const& _m)
{
	std::vector<T> ret;
	for (auto const& i: _m)
		ret.push_back(i.first);
	return ret;
}

template<class T, class U>
std::vector<T> keysOf(std::unordered_map<T, U> const& _m)
{
	std::vector<T> ret;
	for (auto const& i: _m)
		ret.push_back(i.first);
	return ret;
}

template<class T, class U>
std::vector<U> valuesOf(std::map<T, U> const& _m)
{
	std::vector<U> ret;
	ret.reserve(_m.size());
	for (auto const& i: _m)
		ret.push_back(i.second);
	return ret;
}

template<class T, class U>
std::vector<U> valuesOf(std::unordered_map<T, U> const& _m)
{
	std::vector<U> ret;
	ret.reserve(_m.size());
	for (auto const& i: _m)
		ret.push_back(i.second);
	return ret;
}

template <class T, class V>
bool contains(T const& _t, V const& _v)
{
	return std::end(_t) != std::find(std::begin(_t), std::end(_t), _v);
}

}
#endif // SYSCOIN_NEVM_COMMONDATA_H