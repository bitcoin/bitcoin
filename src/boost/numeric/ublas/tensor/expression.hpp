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

#ifndef BOOST_UBLAS_TENSOR_EXPRESSIONS_HPP
#define BOOST_UBLAS_TENSOR_EXPRESSIONS_HPP

#include <cstddef>
#include <boost/numeric/ublas/expression_types.hpp>


namespace boost   {
namespace numeric {
namespace ublas   {


template<class element_type, class storage_format, class storage_type>
class tensor;

template<class size_type>
class basic_extents;


//TODO: put in fwd.hpp
struct tensor_tag {};

}
}
}

namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {

/** @\brief base class for tensor expressions
 *
 * \note implements crtp - no use of virtual function calls
 *
 * \tparam T type of the tensor
 * \tparam E type of the derived expression (crtp)
 *
 **/
template<class T, class E>
struct tensor_expression
    : public ublas_expression<E>
{
	//	static const unsigned complexity = 0;
	using expression_type = E;
	using type_category = tensor_tag;
	using tensor_type = T;

	BOOST_UBLAS_INLINE
	auto const& operator()() const { return *static_cast<const expression_type*> (this); }

protected :
	explicit tensor_expression() = default;
	tensor_expression(const tensor_expression&) = delete;
	tensor_expression& operator=(const tensor_expression&) = delete;
};


template<class T, class EL, class ER, class OP>
struct binary_tensor_expression
    : public tensor_expression <T, binary_tensor_expression<T,EL,ER,OP>>
{
	using self_type = binary_tensor_expression<T,EL,ER,OP>;
	using tensor_type  = T;
	using binary_operation = OP;
	using expression_type_left  = EL;
	using expression_type_right = ER;
	using derived_type =  tensor_expression <tensor_type,self_type>;

	using size_type = typename tensor_type::size_type;

	explicit binary_tensor_expression(expression_type_left  const& l, expression_type_right const& r, binary_operation o)
	  : el(l) , er(r) , op(o) {}
	binary_tensor_expression() = delete;
	binary_tensor_expression(const binary_tensor_expression& l) = delete;
	binary_tensor_expression(binary_tensor_expression&& l)
	  : el(l.el), er(l.er), op(l.op) {}

	BOOST_UBLAS_INLINE
	decltype(auto)  operator()(size_type i) const { return op(el(i), er(i)); }

	expression_type_left const& el;
	expression_type_right const& er;
	binary_operation op;
};

/// @brief helper function to simply instantiation of lambda proxy class
template<class T, class EL, class ER, class OP>
auto make_binary_tensor_expression( tensor_expression<T,EL> const& el, tensor_expression<T,ER> const& er, OP op)
{
	return binary_tensor_expression<T,EL,ER,OP>( el(), er(), op) ;
}

template<class T, class EL, class ER, class OP>
auto make_binary_tensor_expression( matrix_expression<EL> const& el, tensor_expression<T,ER> const& er, OP op)
{
	return binary_tensor_expression<T,EL,ER,OP>( el(), er(), op) ;
}

template<class T, class EL, class ER, class OP>
auto make_binary_tensor_expression( tensor_expression<T,EL> const& el, matrix_expression<ER> const& er, OP op)
{
	return binary_tensor_expression<T,EL,ER,OP>( el(), er(), op) ;
}

template<class T, class EL, class ER, class OP>
auto make_binary_tensor_expression( vector_expression<EL> const& el, tensor_expression<T,ER> const& er, OP op)
{
	return binary_tensor_expression<T,EL,ER,OP>( el(), er(), op) ;
}

template<class T, class EL, class ER, class OP>
auto make_binary_tensor_expression( tensor_expression<T,EL> const& el, vector_expression<ER> const& er, OP op)
{
	return binary_tensor_expression<T,EL,ER,OP>( el(), er(), op) ;
}



template<class T, class E, class OP>
struct unary_tensor_expression
    : public tensor_expression <T, unary_tensor_expression<T,E,OP>>
{

	using self_type = unary_tensor_expression<T,E,OP>;
	using tensor_type  = T;
	using expression_type = E;

	using derived_type = tensor_expression <T, unary_tensor_expression<T,E,OP>>;

	using size_type = typename tensor_type::size_type;

	explicit unary_tensor_expression(E const& ee, OP o) : e(ee) , op(o) {}
	unary_tensor_expression() = delete;
	unary_tensor_expression(const unary_tensor_expression& l) = delete;
	unary_tensor_expression(unary_tensor_expression&& l)
	  : e(l.e), op(op.l) {}

	BOOST_UBLAS_INLINE
	decltype(auto) operator()(size_type i) const { return op(e(i)); }

	E const& e;
	OP op;
};

// \brief helper function to simply instantiation of lambda proxy class
template<class T, class E, class OP>
auto make_unary_tensor_expression( tensor_expression<T,E> const& e, OP op)
{
	return unary_tensor_expression<T,E,OP>( e() , op);
}

template<class T, class E, class OP>
auto make_unary_tensor_expression( matrix_expression<E> const& e, OP op)
{
	return unary_tensor_expression<T,E,OP>( e() , op);
}

template<class T, class E, class OP>
auto make_unary_tensor_expression( vector_expression<E> const& e, OP op)
{
	return unary_tensor_expression<T,E,OP>( e() , op);
}


}
}
}
}
#endif
