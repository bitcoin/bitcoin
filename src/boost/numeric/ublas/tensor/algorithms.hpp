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


#ifndef _BOOST_UBLAS_TENSOR_ALGORITHMS_HPP
#define _BOOST_UBLAS_TENSOR_ALGORITHMS_HPP


#include <stdexcept>
#include <complex>
#include <functional>

namespace boost {
namespace numeric {
namespace ublas {



/** @brief Copies a tensor to another tensor with different layouts
 *
 * Implements C[i1,i2,...,ip] = A[i1,i2,...,ip]
 *
 * @param[in]  p rank of input and output tensor
 * @param[in]  n pointer to the extents of input or output tensor of length p
 * @param[in] pi pointer to a one-based permutation tuple of length p
 * @param[out] c pointer to the output tensor
 * @param[in] wc pointer to the strides of output tensor c
 * @param[in]  a pointer to the input tensor
 * @param[in] wa pointer to the strides of input tensor a
*/
template <class	PointerOut, class PointerIn, class SizeType>
void copy(const SizeType p, SizeType const*const n,
          PointerOut c, SizeType const*const wc,
          PointerIn a,  SizeType const*const wa)
{
	static_assert( std::is_pointer<PointerOut>::value & std::is_pointer<PointerIn>::value,
	               "Static error in boost::numeric::ublas::copy: Argument types for pointers are not pointer types.");
	if( p == 0 )
		return;

	if(c == nullptr || a == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::copy: Pointers shall not be null pointers.");

	if(wc == nullptr || wa == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::copy: Pointers shall not be null pointers.");

	if(n == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::copy: Pointers shall not be null pointers.");


	std::function<void(SizeType r, PointerOut c, PointerIn  a)> lambda;

	lambda = [&lambda, n, wc, wa](SizeType r, PointerOut c, PointerIn a)
	{
		if(r > 0)
			for(auto d = 0u; d < n[r]; c += wc[r], a += wa[r], ++d)
				lambda(r-1, c, a );
		else
			for(auto d = 0u; d < n[0]; c += wc[0], a += wa[0], ++d)
				*c = *a;
	};

	lambda( p-1, c, a );
}



/** @brief Copies a tensor to another tensor with different layouts applying a unary operation
 *
 * Implements C[i1,i2,...,ip] = op ( A[i1,i2,...,ip] )
 *
 * @param[in]  p rank of input and output tensor
 * @param[in]  n pointer to the extents of input or output tensor of length p
 * @param[in] pi pointer to a one-based permutation tuple of length p
 * @param[out] c pointer to the output tensor
 * @param[in] wc pointer to the strides of output tensor c
 * @param[in]  a pointer to the input tensor
 * @param[in] wa pointer to the strides of input tensor a
 * @param[in] op unary operation
*/
template <class	PointerOut, class PointerIn, class SizeType, class UnaryOp>
void transform(const SizeType p,
               SizeType const*const n,
               PointerOut c, SizeType const*const wc,
               PointerIn a,  SizeType const*const wa,
               UnaryOp op)
{
	static_assert( std::is_pointer<PointerOut>::value & std::is_pointer<PointerIn>::value,
	               "Static error in boost::numeric::ublas::transform: Argument types for pointers are not pointer types.");
	if( p == 0 )
		return;

	if(c == nullptr || a == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(wc == nullptr || wa == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(n == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");


	std::function<void(SizeType r, PointerOut c, PointerIn a)> lambda;

	lambda = [&lambda, n, wc, wa, op](SizeType r, PointerOut c, PointerIn a)
	{
		if(r > 0)
			for(auto d = 0u; d < n[r]; c += wc[r], a += wa[r], ++d)
				lambda(r-1, c, a);
		else
			for(auto d = 0u; d < n[0]; c += wc[0], a += wa[0], ++d)
				*c = op(*a);
	};

	lambda( p-1, c, a );

}


/** @brief Performs a reduce operation with all elements of the tensor and an initial value
 *
 * Implements k = sum_{i1,..,ip} A[i1,i2,...,ip]
 *
 * @param[in] r  zero-based recursion level starting with r=p-1
 * @param[in] n  pointer to the extents of input or output tensor
 * @param[in] a  pointer to the first input tensor
 * @param[in] w  pointer to the strides of input tensor a
 * @param[in] k  accumulated value
*/
template <class PointerIn, class ValueType, class SizeType>
ValueType accumulate(SizeType const p, SizeType const*const n,
                     PointerIn a, SizeType const*const w,
                     ValueType k)
{
	static_assert(std::is_pointer<PointerIn>::value,
	              "Static error in boost::numeric::ublas::transform: Argument types for pointers are not pointer types.");

	if( p == 0 )
		return k;

	if(a == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(w == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(n == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");


	std::function<ValueType(SizeType r, PointerIn a, ValueType k)> lambda;

	lambda = [&lambda, n, w](SizeType r, PointerIn a, ValueType k)
	{
		if(r > 0u)
			for(auto d = 0u; d < n[r]; a += w[r], ++d)
				k = lambda(r-1, a, k);
		else
			for(auto d = 0u; d < n[0]; a += w[0], ++d)
				k += *a;
		return k;
	};

	return lambda( p-1, a,  k );
}

/** @brief Performs a reduce operation with all elements of the tensor and an initial value
 *
 * Implements k = op ( k , A[i1,i2,...,ip] ), for all ir
 *
 * @param[in] r  zero-based recursion level starting with r=p-1
 * @param[in] n  pointer to the extents of input or output tensor
 * @param[in] a  pointer to the first input tensor
 * @param[in] w  pointer to the strides of input tensor a
 * @param[in] k  accumulated value
 * @param[in] op binary operation
*/

template <class PointerIn, class ValueType, class SizeType, class BinaryOp>
ValueType accumulate(SizeType const p, SizeType const*const n,
                     PointerIn a, SizeType const*const w,
                     ValueType k, BinaryOp op)
{
	static_assert(std::is_pointer<PointerIn>::value,
	              "Static error in boost::numeric::ublas::transform: Argument types for pointers are not pointer types.");


	if( p == 0 )
		return k;

	if(a == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(w == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");

	if(n == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::transform: Pointers shall not be null pointers.");


	std::function<ValueType(SizeType r, PointerIn a, ValueType k)> lambda;

	lambda = [&lambda, n, w, op](SizeType r, PointerIn a, ValueType k)
	{
		if(r > 0u)
			for(auto d = 0u; d < n[r]; a += w[r], ++d)
				k = lambda(r-1, a, k);
		else
			for(auto d = 0u; d < n[0]; a += w[0], ++d)
				k = op ( k, *a );
		return k;
	};

	return lambda( p-1, a,  k );
}

/** @brief Transposes a tensor
 *
 * Implements C[tau[i1],tau[i2],...,tau[ip]] = A[i1,i2,...,ip]
 *
 * @note is used in function trans
 *
 * @param[in]  p rank of input and output tensor
 * @param[in] na pointer to the extents of the input tensor a of length p
 * @param[in] pi pointer to a one-based permutation tuple of length p
 * @param[out] c pointer to the output tensor
 * @param[in] wc pointer to the strides of output tensor c
 * @param[in]  a pointer to the input tensor
 * @param[in] wa pointer to the strides of input tensor a
*/

template <class PointerOut, class PointerIn, class SizeType>
void trans( SizeType const p,  SizeType const*const na, SizeType const*const pi,
            PointerOut c,      SizeType const*const wc,
            PointerIn a,       SizeType const*const wa)
{

	static_assert( std::is_pointer<PointerOut>::value & std::is_pointer<PointerIn>::value,
	               "Static error in boost::numeric::ublas::trans: Argument types for pointers are not pointer types.");

	if( p < 2)
		return;

	if(c == nullptr || a == nullptr)
		throw std::runtime_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(na == nullptr)
		throw std::runtime_error("Error in boost::numeric::ublas::trans: Pointers shall not be null.");

	if(wc == nullptr || wa == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(na == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(pi == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");


	std::function<void(SizeType r, PointerOut c, PointerIn a)> lambda;

	lambda = [&lambda, na, wc, wa, pi](SizeType r, PointerOut c, PointerIn a)
	{
		if(r > 0)
			for(auto d = 0u; d < na[r]; c += wc[pi[r]-1], a += wa[r], ++d)
				lambda(r-1, c, a);
		else
			for(auto d = 0u; d < na[0]; c += wc[pi[0]-1], a += wa[0], ++d)
				*c = *a;
	};

	lambda( p-1, c, a );
}


/** @brief Transposes a tensor
 *
 * Implements C[tau[i1],tau[i2],...,tau[ip]] = A[i1,i2,...,ip]
 *
 * @note is used in function trans
 *
 * @param[in]  p rank of input and output tensor
 * @param[in] na pointer to the extents of the input tensor a of length p
 * @param[in] pi pointer to a one-based permutation tuple of length p
 * @param[out] c pointer to the output tensor
 * @param[in] wc pointer to the strides of output tensor c
 * @param[in]  a pointer to the input tensor
 * @param[in] wa pointer to the strides of input tensor a
*/

template <class ValueType, class SizeType>
void trans( SizeType const p,
            SizeType const*const na,
            SizeType const*const pi,
            std::complex<ValueType>* c,  SizeType const*const wc,
            std::complex<ValueType>* a,  SizeType const*const wa)
{
	if( p < 2)
		return;

	if(c == nullptr || a == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(wc == nullptr || wa == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(na == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");

	if(pi == nullptr)
		throw std::length_error("Error in boost::numeric::ublas::trans: Pointers shall not be null pointers.");


	std::function<void(SizeType r, std::complex<ValueType>* c, std::complex<ValueType>* a)> lambda;

	lambda = [&lambda, na, wc, wa, pi](SizeType r, std::complex<ValueType>* c, std::complex<ValueType>* a)
	{
		if(r > 0)
			for(auto d = 0u; d < na[r]; c += wc[pi[r]-1], a += wa[r], ++d)
				lambda(r-1, c, a);
		else
			for(auto d = 0u; d < na[0]; c += wc[pi[0]-1], a += wa[0], ++d)
				*c = std::conj(*a);
	};

	lambda( p-1, c, a );

}




}
}
}

#endif
