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


#ifndef BOOST_NUMERIC_UBLAS_TENSOR_EXTENTS_HPP
#define BOOST_NUMERIC_UBLAS_TENSOR_EXTENTS_HPP

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <cassert>

namespace boost {
namespace numeric {
namespace ublas {


/** @brief Template class for storing tensor extents with runtime variable size.
 *
 * Proxy template class of std::vector<int_type>.
 *
 */
template<class int_type>
class basic_extents
{
	static_assert( std::numeric_limits<typename std::vector<int_type>::value_type>::is_integer, "Static error in basic_layout: type must be of type integer.");
	static_assert(!std::numeric_limits<typename std::vector<int_type>::value_type>::is_signed,  "Static error in basic_layout: type must be of type unsigned integer.");

public:
	using base_type = std::vector<int_type>;
	using value_type = typename base_type::value_type;
	using const_reference = typename base_type::const_reference;
	using reference = typename base_type::reference;
	using size_type = typename base_type::size_type;
	using const_pointer = typename base_type::const_pointer;
	using const_iterator = typename base_type::const_iterator;


	/** @brief Default constructs basic_extents
	 *
	 * @code auto ex = basic_extents<unsigned>{};
	 */
	constexpr explicit basic_extents()
	  : _base{}
	{
	}

	/** @brief Copy constructs basic_extents from a one-dimensional container
	 *
	 * @code auto ex = basic_extents<unsigned>(  std::vector<unsigned>(3u,3u) );
	 *
	 * @note checks if size > 1 and all elements > 0
	 *
	 * @param b one-dimensional std::vector<int_type> container
	 */
	explicit basic_extents(base_type const& b)
	  : _base(b)
	{
		if (!this->valid()){
			throw std::length_error("Error in basic_extents::basic_extents() : shape tuple is not a valid permutation: has zero elements.");
		}
	}

	/** @brief Move constructs basic_extents from a one-dimensional container
	 *
	 * @code auto ex = basic_extents<unsigned>(  std::vector<unsigned>(3u,3u) );
	 *
	 * @note checks if size > 1 and all elements > 0
	 *
	 * @param b one-dimensional container of type std::vector<int_type>
	 */
	explicit basic_extents(base_type && b)
	  : _base(std::move(b))
	{
		if (!this->valid()){
			throw std::length_error("Error in basic_extents::basic_extents() : shape tuple is not a valid permutation: has zero elements.");
		}
	}

	/** @brief Constructs basic_extents from an initializer list
	 *
	 * @code auto ex = basic_extents<unsigned>{3,2,4};
	 *
	 * @note checks if size > 1 and all elements > 0
	 *
	 * @param l one-dimensional list of type std::initializer<int_type>
	 */
	basic_extents(std::initializer_list<value_type> l)
	  : basic_extents( base_type(std::move(l)) )
	{
	}

	/** @brief Constructs basic_extents from a range specified by two iterators
	 *
	 * @code auto ex = basic_extents<unsigned>(a.begin(), a.end());
	 *
	 * @note checks if size > 1 and all elements > 0
	 *
	 * @param first iterator pointing to the first element
	 * @param last iterator pointing to the next position after the last element
	 */
	basic_extents(const_iterator first, const_iterator last)
	  : basic_extents ( base_type( first,last ) )
	{
	}

	/** @brief Copy constructs basic_extents */
	basic_extents(basic_extents const& l )
	  : _base(l._base)
	{
	}

	/** @brief Move constructs basic_extents */
	basic_extents(basic_extents && l ) noexcept
	  : _base(std::move(l._base))
	{
	}

	~basic_extents() = default;

	basic_extents& operator=(basic_extents other) noexcept
	{
		swap (*this, other);
		return *this;
	}

	friend void swap(basic_extents& lhs, basic_extents& rhs) {
		std::swap(lhs._base   , rhs._base   );
	}



	/** @brief Returns true if this has a scalar shape
	 *
	 * @returns true if (1,1,[1,...,1])
	*/
	bool is_scalar() const
	{
		return
		    _base.size() != 0 &&
		    std::all_of(_base.begin(), _base.end(),
		                [](const_reference a){ return a == 1;});
	}

	/** @brief Returns true if this has a vector shape
	 *
	 * @returns true if (1,n,[1,...,1]) or (n,1,[1,...,1]) with n > 1
	*/
	bool is_vector() const
	{
		if(_base.size() == 0){
			return false;
		}

		if(_base.size() == 1){
			return _base.at(0) > 1;
		}

		auto greater_one = [](const_reference a){ return a >  1;};
		auto equal_one   = [](const_reference a){ return a == 1;};

		return
		    std::any_of(_base.begin(),   _base.begin()+2, greater_one) &&
		    std::any_of(_base.begin(),   _base.begin()+2, equal_one  ) &&
		    std::all_of(_base.begin()+2, _base.end(),     equal_one);
	}

	/** @brief Returns true if this has a matrix shape
	 *
	 * @returns true if (m,n,[1,...,1]) with m > 1 and n > 1
	*/
	bool is_matrix() const
	{
		if(_base.size() < 2){
			return false;
		}

		auto greater_one = [](const_reference a){ return a >  1;};
		auto equal_one   = [](const_reference a){ return a == 1;};

		return
		    std::all_of(_base.begin(),   _base.begin()+2, greater_one) &&
		    std::all_of(_base.begin()+2, _base.end(),     equal_one  );
	}

	/** @brief Returns true if this is has a tensor shape
	 *
	 * @returns true if !empty() && !is_scalar() && !is_vector() && !is_matrix()
	*/
	bool is_tensor() const
	{
		if(_base.size() < 3){
			return false;
		}

		auto greater_one = [](const_reference a){ return a > 1;};

		return std::any_of(_base.begin()+2, _base.end(), greater_one);
	}

	const_pointer data() const
	{
		return this->_base.data();
	}

	const_reference operator[] (size_type p) const
	{
		return this->_base[p];
	}

	const_reference at (size_type p) const
	{
		return this->_base.at(p);
	}

	reference operator[] (size_type p)
	{
		return this->_base[p];
	}

	reference at (size_type p)
	{
		return this->_base.at(p);
	}


	bool empty() const
	{
		return this->_base.empty();
	}

	size_type size() const
	{
		return this->_base.size();
	}

	/** @brief Returns true if size > 1 and all elements > 0 */
	bool valid() const
	{
		return
		    this->size() > 1 &&
		    std::none_of(_base.begin(), _base.end(),
		                 [](const_reference a){ return a == value_type(0); });
	}

	/** @brief Returns the number of elements a tensor holds with this */
	size_type product() const
	{
		if(_base.empty()){
			return 0;
		}

		return std::accumulate(_base.begin(), _base.end(), 1ul, std::multiplies<>());

	}


	/** @brief Eliminates singleton dimensions when size > 2
	 *
	 * squeeze {  1,1} -> {  1,1}
	 * squeeze {  2,1} -> {  2,1}
	 * squeeze {  1,2} -> {  1,2}
	 *
	 * squeeze {1,2,3} -> {  2,3}
	 * squeeze {2,1,3} -> {  2,3}
	 * squeeze {1,3,1} -> {  3,1}
	 *
	*/
	basic_extents squeeze() const
	{
		if(this->size() <= 2){
			return *this;
		}

		auto new_extent = basic_extents{};
		auto insert_iter = std::back_insert_iterator<typename basic_extents::base_type>(new_extent._base);
		std::remove_copy(this->_base.begin(), this->_base.end(), insert_iter ,value_type{1});
		return new_extent;

	}

	void clear()
	{
		this->_base.clear();
	}

	bool operator == (basic_extents const& b) const
	{
		return _base == b._base;
	}

	bool operator != (basic_extents const& b) const
	{
		return !( _base == b._base );
	}

	const_iterator
	begin() const
	{
		return _base.begin();
	}

	const_iterator
	end() const
	{
		return _base.end();
	}

	base_type const& base() const { return _base; }

private:

	base_type _base;

};

using shape = basic_extents<std::size_t>;

} // namespace ublas
} // namespace numeric
} // namespace boost

#endif
