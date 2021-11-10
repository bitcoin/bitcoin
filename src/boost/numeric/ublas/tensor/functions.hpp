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


#ifndef BOOST_UBLAS_TENSOR_FUNCTIONS_HPP
#define BOOST_UBLAS_TENSOR_FUNCTIONS_HPP


#include <stdexcept>
#include <vector>
#include <algorithm>
#include <numeric>


#include "multiplication.hpp"
#include "algorithms.hpp"
#include "expression.hpp"
#include "expression_evaluation.hpp"
#include "storage_traits.hpp"

namespace boost {
namespace numeric {
namespace ublas {

template<class Value, class Format, class Allocator>
class tensor;

template<class Value, class Format, class Allocator>
class matrix;

template<class Value, class Allocator>
class vector;




/** @brief Computes the m-mode tensor-times-vector product
 *
 * Implements C[i1,...,im-1,im+1,...,ip] = A[i1,i2,...,ip] * b[im]
 *
 * @note calls ublas::ttv
 *
 * @param[in] m contraction dimension with 1 <= m <= p
 * @param[in] a tensor object A with order p
 * @param[in] b vector object B
 *
 * @returns tensor object C with order p-1, the same storage format and allocator type as A
*/
template<class V, class F, class A1, class A2>
auto prod(tensor<V,F,A1> const& a, vector<V,A2> const& b, const std::size_t m)
{

	using tensor_type  = tensor<V,F,A1>;
	using extents_type = typename tensor_type::extents_type;
	using ebase_type   = typename extents_type::base_type;
	using value_type   = typename tensor_type::value_type;
	using size_type = typename extents_type::value_type;

	auto const p = std::size_t(a.rank());
	
	if( m == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttv): contraction mode must be greater than zero.");

	if( p < m )
		throw std::length_error("error in boost::numeric::ublas::prod(ttv): rank of tensor must be greater than or equal to the modus.");

	if( p == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttv): rank of tensor must be greater than zero.");

	if( a.empty() )
		throw std::length_error("error in boost::numeric::ublas::prod(ttv): first argument tensor should not be empty.");

	if( b.size() == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttv): second argument vector should not be empty.");


	auto nc = ebase_type(std::max(p-1, size_type(2)) , size_type(1));
	auto nb = ebase_type{b.size(),1};


	for(auto i = 0u, j = 0u; i < p; ++i)
		if(i != m-1)
			nc[j++] = a.extents().at(i);

	auto c = tensor_type(extents_type(nc),value_type{});

	auto bb = &(b(0));

	ttv(m, p,
	    c.data(), c.extents().data(), c.strides().data(),
	    a.data(), a.extents().data(), a.strides().data(),
	    bb, nb.data(), nb.data());


	return c;
}



/** @brief Computes the m-mode tensor-times-matrix product
 *
 * Implements C[i1,...,im-1,j,im+1,...,ip] = A[i1,i2,...,ip] * B[j,im]
 *
 * @note calls ublas::ttm
 *
 * @param[in] a tensor object A with order p
 * @param[in] b vector object B
 * @param[in] m contraction dimension with 1 <= m <= p
 *
 * @returns tensor object C with order p, the same storage format and allocator type as A
*/
template<class V, class F, class A1, class A2>
auto prod(tensor<V,F,A1> const& a, matrix<V,F,A2> const& b, const std::size_t m)
{

	using tensor_type  = tensor<V,F,A1>;
	using extents_type = typename tensor_type::extents_type;
	using strides_type = typename tensor_type::strides_type;
	using value_type   = typename tensor_type::value_type;


	auto const p = a.rank();

	if( m == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttm): contraction mode must be greater than zero.");

	if( p < m || m > a.extents().size())
		throw std::length_error("error in boost::numeric::ublas::prod(ttm): rank of the tensor must be greater equal the modus.");

	if( p == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttm): rank of the tensor must be greater than zero.");

	if( a.empty() )
		throw std::length_error("error in boost::numeric::ublas::prod(ttm): first argument tensor should not be empty.");

	if( b.size1()*b.size2() == 0)
		throw std::length_error("error in boost::numeric::ublas::prod(ttm): second argument matrix should not be empty.");


	auto nc = a.extents().base();
	auto nb = extents_type {b.size1(),b.size2()};
	auto wb = strides_type (nb);

	nc[m-1] = nb[0];

	auto c = tensor_type(extents_type(nc),value_type{});

	auto bb = &(b(0,0));

	ttm(m, p,
	    c.data(), c.extents().data(), c.strides().data(),
	    a.data(), a.extents().data(), a.strides().data(),
	    bb, nb.data(), wb.data());


	return c;
}




/** @brief Computes the q-mode tensor-times-tensor product
 *
 * Implements C[i1,...,ir,j1,...,js] = sum( A[i1,...,ir+q] * B[j1,...,js+q]  )
 *
 * @note calls ublas::ttt
 *
 * na[phia[x]] = nb[phib[x]] for 1 <= x <= q
 *
 * @param[in]	 phia one-based permutation tuple of length q for the first input tensor a
 * @param[in]	 phib one-based permutation tuple of length q for the second input tensor b
 * @param[in]  a  left-hand side tensor with order r+q
 * @param[in]  b  right-hand side tensor with order s+q
 * @result     tensor with order r+s
*/
template<class V, class F, class A1, class A2>
auto prod(tensor<V,F,A1> const& a, tensor<V,F,A2> const& b,
          std::vector<std::size_t> const& phia, std::vector<std::size_t> const& phib)
{

	using tensor_type  = tensor<V,F,A1>;
	using extents_type = typename tensor_type::extents_type;
	using value_type   = typename tensor_type::value_type;
	using size_type = typename extents_type::value_type;

	auto const pa = a.rank();
	auto const pb = b.rank();

	auto const q  = size_type(phia.size());

	if(pa == 0ul)
		throw std::runtime_error("error in ublas::prod: order of left-hand side tensor must be greater than 0.");
	if(pb == 0ul)
		throw std::runtime_error("error in ublas::prod: order of right-hand side tensor must be greater than 0.");
	if(pa < q)
		throw std::runtime_error("error in ublas::prod: number of contraction dimensions cannot be greater than the order of the left-hand side tensor.");
	if(pb < q)
		throw std::runtime_error("error in ublas::prod: number of contraction dimensions cannot be greater than the order of the right-hand side tensor.");

	if(q != phib.size())
		throw std::runtime_error("error in ublas::prod: permutation tuples must have the same length.");

	if(pa < phia.size())
		throw std::runtime_error("error in ublas::prod: permutation tuple for the left-hand side tensor cannot be greater than the corresponding order.");
	if(pb < phib.size())
		throw std::runtime_error("error in ublas::prod: permutation tuple for the right-hand side tensor cannot be greater than the corresponding order.");


	auto const& na = a.extents();
	auto const& nb = b.extents();

	for(auto i = 0ul; i < q; ++i)
		if( na.at(phia.at(i)-1) != nb.at(phib.at(i)-1))
			throw std::runtime_error("error in ublas::prod: permutations of the extents are not correct.");

	auto const r = pa - q;
	auto const s = pb - q;


	std::vector<std::size_t> phia1(pa), phib1(pb);
	std::iota(phia1.begin(), phia1.end(), 1ul);
	std::iota(phib1.begin(), phib1.end(), 1ul);

	std::vector<std::size_t> nc( std::max ( r+s , size_type(2) ), size_type(1) );

	for(auto i = 0ul; i < phia.size(); ++i)
		* std::remove(phia1.begin(), phia1.end(), phia.at(i)) = phia.at(i);

	//phia1.erase( std::remove(phia1.begin(), phia1.end(), phia.at(i)),  phia1.end() )  ;

	assert(phia1.size() == pa);

	for(auto i = 0ul; i < r; ++i)
		nc[ i ] = na[ phia1[ i  ] - 1  ];

	for(auto i = 0ul; i < phib.size(); ++i)
		* std::remove(phib1.begin(), phib1.end(), phib.at(i))  = phib.at(i) ;
	//phib1.erase( std::remove(phib1.begin(), phib1.end(), phia.at(i)), phib1.end() )  ;

	assert(phib1.size() == pb);

	for(auto i = 0ul; i < s; ++i)
		nc[ r + i ] = nb[ phib1[ i  ] - 1  ];

	//	std::copy( phib.begin(), phib.end(), phib1.end()  );

	assert(  phia1.size() == pa  );
	assert(  phib1.size() == pb  );

	auto c = tensor_type(extents_type(nc), value_type{});

	ttt(pa, pb, q,
	    phia1.data(), phib1.data(),
	    c.data(), c.extents().data(), c.strides().data(),
	    a.data(), a.extents().data(), a.strides().data(),
	    b.data(), b.extents().data(), b.strides().data());

	return c;
}

//template<class V, class F, class A1, class A2, std::size_t N, std::size_t M>
//auto operator*( tensor_index<V,F,A1,N> const& lhs, tensor_index<V,F,A2,M> const& rhs)




/** @brief Computes the q-mode tensor-times-tensor product
 *
 * Implements C[i1,...,ir,j1,...,js] = sum( A[i1,...,ir+q] * B[j1,...,js+q]  )
 *
 * @note calls ublas::ttt
 *
 * na[phi[x]] = nb[phi[x]] for 1 <= x <= q
 *
 * @param[in]	 phi one-based permutation tuple of length q for bot input tensors
 * @param[in]  a  left-hand side tensor with order r+q
 * @param[in]  b  right-hand side tensor with order s+q
 * @result     tensor with order r+s
*/
template<class V, class F, class A1, class A2>
auto prod(tensor<V,F,A1> const& a, tensor<V,F,A2> const& b,
          std::vector<std::size_t> const& phi)
{
	return prod(a, b, phi, phi);
}


/** @brief Computes the inner product of two tensors
 *
 * Implements c = sum(A[i1,i2,...,ip] * B[i1,i2,...,jp])
 *
 * @note calls inner function
 *
 * @param[in] a tensor object A
 * @param[in] b tensor object B
 *
 * @returns a value type.
*/
template<class V, class F, class A1, class A2>
auto inner_prod(tensor<V,F,A1> const& a, tensor<V,F,A2> const& b)
{
	using value_type   = typename tensor<V,F,A1>::value_type;

	if( a.rank() != b.rank() )
		throw std::length_error("error in boost::numeric::ublas::inner_prod: Rank of both tensors must be the same.");

	if( a.empty() || b.empty())
		throw std::length_error("error in boost::numeric::ublas::inner_prod: Tensors should not be empty.");

	if( a.extents() != b.extents())
		throw std::length_error("error in boost::numeric::ublas::inner_prod: Tensor extents should be the same.");

	return inner(a.rank(), a.extents().data(),
	             a.data(), a.strides().data(),
	             b.data(), b.strides().data(), value_type{0});
}

/** @brief Computes the outer product of two tensors
 *
 * Implements C[i1,...,ip,j1,...,jq] = A[i1,i2,...,ip] * B[j1,j2,...,jq]
 *
 * @note calls outer function
 *
 * @param[in] a tensor object A
 * @param[in] b tensor object B
 *
 * @returns tensor object C with the same storage format F and allocator type A1
*/
template<class V, class F, class A1, class A2>
auto outer_prod(tensor<V,F,A1> const& a, tensor<V,F,A2> const& b)
{
	using tensor_type  = tensor<V,F,A1>;
	using extents_type = typename tensor_type::extents_type;

	if( a.empty() || b.empty() )
		throw std::runtime_error("error in boost::numeric::ublas::outer_prod: tensors should not be empty.");

	auto nc = typename extents_type::base_type(a.rank() + b.rank());
	for(auto i = 0u; i < a.rank(); ++i)
		nc.at(i) = a.extents().at(i);

	for(auto i = 0u; i < b.rank(); ++i)
		nc.at(a.rank()+i) = b.extents().at(i);

	auto c = tensor_type(extents_type(nc));

	outer(c.data(), c.rank(), c.extents().data(), c.strides().data(),
	      a.data(), a.rank(), a.extents().data(), a.strides().data(),
	      b.data(), b.rank(), b.extents().data(), b.strides().data());

	return c;
}



/** @brief Transposes a tensor according to a permutation tuple
 *
 * Implements C[tau[i1],tau[i2]...,tau[ip]] = A[i1,i2,...,ip]
 *
 * @note calls trans function
 *
 * @param[in] a    tensor object of rank p
 * @param[in] tau  one-based permutation tuple of length p
 * @returns        a transposed tensor object with the same storage format F and allocator type A
*/
template<class V, class F, class A>
auto trans(tensor<V,F,A> const& a, std::vector<std::size_t> const& tau)
{
	using tensor_type  = tensor<V,F,A>;
	using extents_type = typename tensor_type::extents_type;
	//	using strides_type = typename tensor_type::strides_type;

	if( a.empty() )
		return tensor<V,F,A>{};

	auto const   p = a.rank();
	auto const& na = a.extents();

	auto nc = typename extents_type::base_type (p);
	for(auto i = 0u; i < p; ++i)
		nc.at(tau.at(i)-1) = na.at(i);

	//	auto wc = strides_type(extents_type(nc));

	auto c = tensor_type(extents_type(nc));


	trans( a.rank(), a.extents().data(), tau.data(),
	       c.data(), c.strides().data(),
	       a.data(), a.strides().data());

	//	auto wc_pi = typename strides_type::base_type (p);
	//	for(auto i = 0u; i < p; ++i)
	//		wc_pi.at(tau.at(i)-1) = c.strides().at(i);


	//copy(a.rank(),
	//		 a.extents().data(),
	//		 c.data(), wc_pi.data(),
	//		 a.data(), a.strides().data() );

	return c;
}

/** @brief Computes the frobenius norm of a tensor expression
 *
 * @note evaluates the tensor expression and calls the accumulate function
 *
 *
 * Implements the two-norm with
 * k = sqrt( sum_(i1,...,ip) A(i1,...,ip)^2 )
 *
 * @param[in] a    tensor object of rank p
 * @returns        the frobenius norm of the tensor
*/
//template<class V, class F, class A>
//auto norm(tensor<V,F,A> const& a)
template<class T, class D>
auto norm(detail::tensor_expression<T,D> const& expr)
{

	using tensor_type = typename detail::tensor_expression<T,D>::tensor_type;
	using value_type = typename tensor_type::value_type;

	auto a = tensor_type( expr );

	if( a.empty() )
		throw std::runtime_error("error in boost::numeric::ublas::norm: tensors should not be empty.");

	return std::sqrt( accumulate( a.order(), a.extents().data(), a.data(), a.strides().data(), value_type{},
	                              [](auto const& l, auto const& r){ return l + r*r; }  ) ) ;
}



/** @brief Extract the real component of tensor elements within a tensor expression
 *
 * @param[in] lhs tensor expression
 * @returns   unary tensor expression
*/
template<class T, class D>
auto real(detail::tensor_expression<T,D> const& expr) {
	return detail::make_unary_tensor_expression<T> (expr(), [] (auto const& l) { return std::real( l ); } );
}

/** @brief Extract the real component of tensor elements within a tensor expression
 *
 * @param[in] lhs tensor expression
 * @returns   unary tensor expression
*/
template<class V, class F, class A, class D>
auto real(detail::tensor_expression<tensor<std::complex<V>,F,A>,D> const& expr)
{
	using tensor_complex_type = tensor<std::complex<V>,F,A>;
	using tensor_type = tensor<V,F,typename storage_traits<A>::template rebind<V>>;

	if( detail::retrieve_extents( expr  ).empty() )
		throw std::runtime_error("error in boost::numeric::ublas::real: tensors should not be empty.");

	auto a = tensor_complex_type( expr );
	auto c = tensor_type( a.extents() );

	std::transform( a.begin(), a.end(),  c.begin(), [](auto const& l){ return std::real(l) ; }  );

	return c;
}


/** @brief Extract the imaginary component of tensor elements within a tensor expression
 *
 * @param[in] lhs tensor expression
 * @returns   unary tensor expression
*/
template<class T, class D>
auto imag(detail::tensor_expression<T,D> const& lhs) {
	return detail::make_unary_tensor_expression<T> (lhs(), [] (auto const& l) { return std::imag( l ); } );
}


/** @brief Extract the imag component of tensor elements within a tensor expression
 *
 * @param[in] lhs tensor expression
 * @returns   unary tensor expression
*/
template<class V, class A, class F, class D>
auto imag(detail::tensor_expression<tensor<std::complex<V>,F,A>,D> const& expr)
{
	using tensor_complex_type = tensor<std::complex<V>,F,A>;
	using tensor_type = tensor<V,F,typename storage_traits<A>::template rebind<V>>;

	if( detail::retrieve_extents( expr  ).empty() )
		throw std::runtime_error("error in boost::numeric::ublas::real: tensors should not be empty.");

	auto a = tensor_complex_type( expr );
	auto c = tensor_type( a.extents() );

	std::transform( a.begin(), a.end(),  c.begin(), [](auto const& l){ return std::imag(l) ; }  );

	return c;
}

/** @brief Computes the complex conjugate component of tensor elements within a tensor expression
 *
 * @param[in] expr tensor expression
 * @returns   complex tensor
*/
template<class T, class D>
auto conj(detail::tensor_expression<T,D> const& expr)
{
	using tensor_type = T;
	using value_type = typename tensor_type::value_type;
	using layout_type = typename tensor_type::layout_type;
	using array_type = typename tensor_type::array_type;

	using new_value_type = std::complex<value_type>;
	using new_array_type = typename storage_traits<array_type>::template rebind<new_value_type>;

	using tensor_complex_type = tensor<new_value_type,layout_type, new_array_type>;

	if( detail::retrieve_extents( expr  ).empty() )
		throw std::runtime_error("error in boost::numeric::ublas::conj: tensors should not be empty.");

	auto a = tensor_type( expr );
	auto c = tensor_complex_type( a.extents() );

	std::transform( a.begin(), a.end(),  c.begin(), [](auto const& l){ return std::conj(l) ; }  );

	return c;
}


/** @brief Computes the complex conjugate component of tensor elements within a tensor expression
 *
 * @param[in] lhs tensor expression
 * @returns   unary tensor expression
*/
template<class V, class A, class F, class D>
auto conj(detail::tensor_expression<tensor<std::complex<V>,F,A>,D> const& expr)
{
	return detail::make_unary_tensor_expression<tensor<std::complex<V>,F,A>> (expr(), [] (auto const& l) { return std::conj( l ); } );
}



}
}
}


#endif
