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
/** @file SHA3.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * The FixedHash fixed-size "hash" container type.
 */

#ifndef SYSCOIN_NEVM_SHA3_H
#define SYSCOIN_NEVM_SHA3_H

#include <string>
#include <nevm/fixedhash.h>
#include <nevm/vector_ref.h>

namespace dev
{

// SHA-3 convenience routines.

/// Calculate SHA3-256 hash of the given input and load it into the given output.
/// @returns false if o_output.size() != 32.
bool sha3(bytesConstRef _input, bytesRef o_output);

/// Calculate SHA3-256 hash of the given input, returning as a 256-bit hash.
inline h256 sha3(bytesConstRef _input) { h256 ret; sha3(_input, ret.ref()); return ret; }
inline SecureFixedHash<32> sha3Secure(bytesConstRef _input) { SecureFixedHash<32> ret; sha3(_input, ret.writable().ref()); return ret; }

/// Calculate SHA3-256 hash of the given input, returning as a 256-bit hash.
inline h256 sha3(bytes const& _input) { return sha3(bytesConstRef(&_input)); }
inline SecureFixedHash<32> sha3Secure(bytes const& _input) { return sha3Secure(bytesConstRef(&_input)); }

/// Calculate SHA3-256 hash of the given input (presented as a binary-filled string), returning as a 256-bit hash.
inline h256 sha3(std::string const& _input) { return sha3(bytesConstRef(_input)); }
inline SecureFixedHash<32> sha3Secure(std::string const& _input) { return sha3Secure(bytesConstRef(_input)); }

/// Calculate SHA3-256 hash of the given input (presented as a FixedHash), returns a 256-bit hash.
template<unsigned N> inline h256 sha3(FixedHash<N> const& _input) { return sha3(_input.ref()); }
template<unsigned N> inline SecureFixedHash<32> sha3Secure(FixedHash<N> const& _input) { return sha3Secure(_input.ref()); }

/// Fully secure variants are equivalent for sha3 and sha3Secure.
inline SecureFixedHash<32> sha3(bytesSec const& _input) { return sha3Secure(_input.ref()); }
inline SecureFixedHash<32> sha3Secure(bytesSec const& _input) { return sha3Secure(_input.ref()); }
template<unsigned N> inline SecureFixedHash<32> sha3(SecureFixedHash<N> const& _input) { return sha3Secure(_input.ref()); }
template<unsigned N> inline SecureFixedHash<32> sha3Secure(SecureFixedHash<N> const& _input) { return sha3Secure(_input.ref()); }

/// Calculate SHA3-256 hash of the given input, possibly interpreting it as nibbles, and return the hash as a string filled with binary data.
inline std::string sha3(std::string const& _input, bool _isNibbles) { return asString((_isNibbles ? sha3(fromHex(_input)) : sha3(bytesConstRef(&_input))).asBytes()); }

/// Calculate SHA3-256 MAC
inline void sha3mac(bytesConstRef _secret, bytesConstRef _plain, bytesRef _output) { sha3(_secret.toBytes() + _plain.toBytes()).ref().populate(_output); }

extern h256 EmptySHA3;

extern h256 EmptyListSHA3;

}
#endif // SYSCOIN_NEVM_SHA3_H