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

#ifndef BOOST_UBLAS_TENSOR_MULTI_INDEX_UTILITY_HPP
#define BOOST_UBLAS_TENSOR_MULTI_INDEX_UTILITY_HPP


#include <tuple>
#include <type_traits>


namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {


template<class ... index_types>
struct has_index_impl;

template<class itype_left, class itype_right>
struct has_index_impl<itype_left, itype_right>
{
	static constexpr bool value = itype_left::value == itype_right::value;
};

template<class itype_left>
struct has_index_impl <itype_left, std::tuple<> >
{
	static constexpr bool value = false;
};

template<class itype_left, class itype_right>
struct has_index_impl <itype_left, std::tuple<itype_right> >
{
	static constexpr bool value = has_index_impl<itype_left,itype_right>::value;
};

template<class itype_left, class itype_right, class ... index_types>
struct has_index_impl <itype_left, std::tuple<itype_right, index_types...> >
{
	using next_type = has_index_impl<itype_left, std::tuple<index_types...>>;
	static constexpr bool value = has_index_impl<itype_left,itype_right>::value || next_type::value;
};
} // namespace detail



/** @brief has_index is true if index occurs once or more in a multi-index
 *
 * @note a multi-index represents as tuple of single indexes of type boost::numeric::ublas::index::index_type
 *
 * @code auto has_index_value = has_index<index_type<1>, std::tuple<index_type<2>,index_type<1>> >::value; @endcode
 *
 * @tparam index_type type of index
 * @tparam tuple_type type of std::tuple representing a multi-index
*/
template<class index_type, class tuple_type>
struct has_index
{
	static constexpr bool value = detail::has_index_impl<std::decay_t<index_type>,std::decay_t<tuple_type>>::value;
};

} // namespace ublas
} // namespace numeric
} // namespace boost

////////////////////////////////////////////////
////////////////////////////////////////////////

namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {


template<class ... index_types>
struct valid_multi_index_impl;

template<>
struct valid_multi_index_impl<std::tuple<>>
{
	static constexpr bool value = true;
};

template<class itype>
struct valid_multi_index_impl<std::tuple<itype>>
{
	static constexpr bool value = true;
};


template<class itype, class ... index_types>
struct valid_multi_index_impl<std::tuple<itype,index_types...>>
{
	using ttype     = std::tuple<index_types...>;
	using has_index_type = has_index<itype, ttype>;

	static constexpr bool is_index_zero   = itype::value==0ul;
	static constexpr bool has_index_value = has_index_type::value && !is_index_zero;
	static constexpr bool value = !has_index_value && valid_multi_index_impl<ttype>::value;
};
} // namespace detail

/** @brief valid_multi_index is true if indexes occur only once in a multi-index
 *
 * @note a multi-index represents as tuple of single indexes of type boost::numeric::ublas::index::index_type
 *
 * @code auto valid = valid_multi_index<  std::tuple<index_type<2>,index_type<1>>  >::value;
 * @endcode
 *
 * @tparam tuple_type type of std::tuple representing a multi-index
*/
template<class tupe_type>
struct valid_multi_index
{
	static constexpr bool value = detail::valid_multi_index_impl<std::decay_t<tupe_type>>::value;
};

} // namespace ublas
} // namespace numeric
} // namespace boost

////////////////////////////////////////////////
////////////////////////////////////////////////

namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {

template<class ... index_types >
struct number_equal_indexes_impl;

template<class ... itypes_right >
struct number_equal_indexes_impl < std::tuple<>, std::tuple<itypes_right...>>
{
	static constexpr unsigned value  = 0;
};

template<class itype, class ... itypes_left, class ... itypes_right>
struct number_equal_indexes_impl < std::tuple<itype,itypes_left...>, std::tuple<itypes_right...>>
{
	using tuple_right = std::tuple<itypes_right...>;
	using has_index_type = has_index<itype, tuple_right>;

	static constexpr bool is_index_zero   = itype::value==0ul;
	static constexpr bool has_index_value = has_index_type::value && !is_index_zero;

	using next_type = number_equal_indexes_impl< std::tuple<itypes_left...>, tuple_right >;
	static constexpr unsigned v = has_index_value ? 1 : 0;
	static constexpr unsigned value  = v + next_type::value;
};
} // namespace detail


/** @brief number_equal_indexes contains the number of equal indexes of two multi-indexes
 *
 * @note a multi-index represents as tuple of single indexes of type boost::numeric::ublas::index::index_type
 *
 *
 * @code auto num = number_equal_indexes<
 *                        std::tuple<index_type<2>,index_type<1>>,
 *                        std::tuple<index_type<1>,index_type<3>>  >::value;
 * @endcode
 *
 * @tparam tuple_type_left  type of left std::tuple representing a multi-index
 * @tparam tuple_type_right type of right std::tuple representing a multi-index
*/
template<class tuple_left, class tuple_right>
struct number_equal_indexes
{
	static constexpr unsigned value  =
	    detail::number_equal_indexes_impl< std::decay_t<tuple_left>, std::decay_t<tuple_right>>::value;
};

} // namespace ublas
} // namespace numeric
} // namespace boost


////////////////////////////////////////////////
////////////////////////////////////////////////

namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {


template<std::size_t r, std::size_t m, class itype, class ttype>
struct index_position_impl
{
	static constexpr auto is_same = std::is_same< std::decay_t<itype>, std::decay_t<std::tuple_element_t<r,ttype>> >::value;
	static constexpr auto value   = is_same ? r : index_position_impl<r+1,m,itype,ttype>::value;
};



template<std::size_t m, class itype, class ttype>
struct index_position_impl < m, m, itype, ttype>
{
	static constexpr auto value = std::tuple_size<ttype>::value;
};

} // namespace detail



/** @brief index_position contains the zero-based index position of an index type within a multi-index
 *
 * @note a multi-index represents as tuple of single indexes of type boost::numeric::ublas::index::index_type
 *
 * @code auto num = index_position<
 *                       index_type<1>,
 *                       std::tuple<index_type<2>,index_type<1>>  >::value;
 * @endcode
 *
 * @returns value returns 0 and N-1 if index_type is found, N otherwise where N is tuple_size_v<tuple_type>.
 *
 * @tparam index_type type of index
 * @tparam tuple_type type of std::tuple that is searched for index
*/
template<class index_type, class tuple_type>
struct index_position
{
	static constexpr auto value  = detail::index_position_impl<0ul,std::tuple_size<tuple_type>::value,std::decay_t<index_type>,std::decay_t<tuple_type>>::value;
};

} // namespace ublas
} // namespace numeric
} // namespace boost

////////////////////////////////////////////////
////////////////////////////////////////////////


namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {

template<std::size_t r, std::size_t m>
struct index_position_pairs_impl
{
	template<class array_type, class tuple_left, class tuple_right>
	static constexpr void run(array_type& out, tuple_left const& lhs, tuple_right const& rhs, std::size_t p)
	{
		using index_type     = std::tuple_element_t<r-1,tuple_left>;
		using has_index_type = has_index<index_type, tuple_right>;
		using get_index_type = index_position<index_type,tuple_right>;
		using next_type      = index_position_pairs_impl<r+1,m>;
		if constexpr ( has_index_type::value && index_type::value != 0)
		    out[p++] = std::make_pair(r-1,get_index_type::value);
		next_type::run( out, lhs, rhs, p );
	}
};

template<std::size_t m>
struct index_position_pairs_impl<m,m>
{
	template<class array_type, class tuple_left, class tuple_right>
	static constexpr void run(array_type& out, tuple_left const& , tuple_right const& , std::size_t p)
	{
		using index_type     = std::tuple_element_t<m-1,tuple_left>;
		using has_index_type = has_index<index_type, tuple_right>;
		using get_index_type = index_position<index_type, tuple_right>;
		if constexpr ( has_index_type::value && index_type::value != 0 )
		    out[p] = std::make_pair(m-1,get_index_type::value);
	}
};

template<std::size_t r>
struct index_position_pairs_impl<r,0>
{
	template<class array_type, class tuple_left, class tuple_right>
	static constexpr void run(array_type&, tuple_left const& , tuple_right const& , std::size_t)
	{}
};


} // namespace detail


/** @brief index_position_pairs returns zero-based index positions of matching indexes of two multi-indexes
 *
 * @note a multi-index represents as tuple of single indexes of type boost::numeric::ublas::index::index_type
 *
 * @code auto pairs = index_position_pairs(std::make_tuple(_a,_b), std::make_tuple(_b,_c));
 * @endcode
 *
 * @returns a std::array instance containing index position pairs of type std::pair<std::size_t, std::size_t>.
 *
 * @param lhs left std::tuple instance representing a multi-index
 * @param rhs right std::tuple instance representing a multi-index
*/
template<class tuple_left, class tuple_right>
auto index_position_pairs(tuple_left const& lhs, tuple_right const& rhs)
{
	using pair_type = std::pair<std::size_t,std::size_t>;
	constexpr auto m = std::tuple_size<tuple_left >::value;
	constexpr auto p = number_equal_indexes<tuple_left, tuple_right>::value;
	auto array = std::array<pair_type,p>{};
	detail::index_position_pairs_impl<1,m>::run(array, lhs, rhs,0);
	return array;
}

} // namespace ublas
} // namespace numeric
} // namespace boost

////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////


namespace boost   {
namespace numeric {
namespace ublas   {
namespace detail  {

template<class array_type, std::size_t ... R>
constexpr auto array_to_vector_impl( array_type const& array, std::index_sequence<R...> )
{
	return std::make_pair(
	      std::vector<std::size_t>{std::get<0>( std::get<R>(array) )+1 ...} ,
	      std::vector<std::size_t>{std::get<1>( std::get<R>(array) )+1 ...} );
}

} // namespace detail


/** @brief array_to_vector converts a std::array of zero-based index position pairs into two std::vector of one-based index positions
 *
 * @code auto two_vectors = array_to_vector(std::make_array ( std::make_pair(1,2), std::make_pair(3,4) ) ) ;
 * @endcode
 *
 * @returns two std::vector of one-based index positions
 *
 * @param array std::array of zero-based index position pairs
*/
template<class pair_type, std::size_t N>
constexpr auto array_to_vector( std::array<pair_type,N> const& array)
{
	constexpr auto sequence = std::make_index_sequence<N>{};
	return detail::array_to_vector_impl( array, sequence );
}


} // namespace ublas
} // namespace numeric
} // namespace boost


#endif // _BOOST_UBLAS_TENSOR_MULTI_INDEX_UTILITY_HPP_
