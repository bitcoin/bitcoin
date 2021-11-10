//
//  Copyright (c) 2018-2019, Cem Bassoy, cem.bassoy@gmail.com
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  Fraunhofer IOSB, Ettlingen, Germany
//

#ifndef BOOST_UBLAS_TENSOR_INDEX_HPP
#define BOOST_UBLAS_TENSOR_INDEX_HPP


#include <cstddef>
#include <array>
#include <vector>

namespace boost {
namespace numeric {
namespace ublas {
namespace index {

/** @brief Proxy template class for the einstein summation notation
 *
 * @note index::index_type<K> for 0<=K<=16 is used in tensor::operator()
 *
 * @tparam I wrapped integer
*/
template<std::size_t I>
struct index_type
{
	static constexpr std::size_t value = I;

	constexpr bool operator == (std::size_t other) const { return value == other; }
	constexpr bool operator != (std::size_t other) const { return value != other; }

	template <std::size_t K>
	constexpr bool operator == (index_type<K> /*other*/) const {  return I==K; }
	template <std::size_t  K>
	constexpr bool operator != (index_type<K> /*other*/) const {  return I!=K; }

	constexpr bool operator == (index_type /*other*/) const {  return true;  }
	constexpr bool operator != (index_type /*other*/) const {  return false; }

	constexpr std::size_t operator()() const { return I; }
};

/** @brief Proxy classes for the einstein summation notation
 *
 * @note index::_a ... index::_z is used in tensor::operator()
*/

static constexpr index_type< 0> _;
static constexpr index_type< 1> _a;
static constexpr index_type< 2> _b;
static constexpr index_type< 3> _c;
static constexpr index_type< 4> _d;
static constexpr index_type< 5> _e;
static constexpr index_type< 6> _f;
static constexpr index_type< 7> _g;
static constexpr index_type< 8> _h;
static constexpr index_type< 9> _i;
static constexpr index_type<10> _j;
static constexpr index_type<11> _k;
static constexpr index_type<12> _l;
static constexpr index_type<13> _m;
static constexpr index_type<14> _n;
static constexpr index_type<15> _o;
static constexpr index_type<16> _p;
static constexpr index_type<17> _q;
static constexpr index_type<18> _r;
static constexpr index_type<19> _s;
static constexpr index_type<20> _t;
static constexpr index_type<21> _u;
static constexpr index_type<22> _v;
static constexpr index_type<23> _w;
static constexpr index_type<24> _x;
static constexpr index_type<25> _y;
static constexpr index_type<26> _z;

} // namespace indices

}
}
}

#endif // _BOOST_UBLAS_TENSOR_INDEX_HPP_
