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

#ifndef BOOST_UBLAS_TENSOR_OSTREAM_HPP
#define BOOST_UBLAS_TENSOR_OSTREAM_HPP

#include <ostream>
#include <complex>

namespace boost {
namespace numeric {
namespace ublas {
namespace detail {

template <class value_type>
void print(std::ostream& out, value_type const& p)
{
	out << p << " ";
}

template <class value_type>
void print(std::ostream& out, const std::complex<value_type>& p)
{
	out << std::real(p) << "+" << std::imag(p) << "i ";
}


template <class size_type, class value_type>
void print(std::ostream& out, size_type r, const value_type* p, const size_type* w, const size_type* n)
{

	if(r < 2)
	{
		out << "[ ... " << std::endl;

		for(auto row = 0u; row < n[0]; p += w[0], ++row) // iterate over one column
		{
			auto p1 = p;
			for(auto col = 0u; col < n[1]; p1 += w[1], ++col) // iterate over first row
			{
				print(out,*p1);
			}
			if(row < n[0]-1)
				out << "; " << std::endl;
		}
		out << "]";
	}
	else
	{
		out << "cat("<< r+1 <<",..." << std::endl;
		for(auto d = 0u; d < n[r]-1; p += w[r], ++d){
			print(out, r-1, p, w, n);
			out << ",..." << std::endl;
		}
		print(out, r-1, p, w, n);
	}
	if(r>1)
		out << ")";
}

////////////////////////////


}
}
}
}


namespace boost {
namespace numeric {
namespace ublas {

template<class T, class F, class A>
class tensor;

template<class T, class F, class A>
class matrix;

template<class T, class A>
class vector;

}
}
}


template <class V, class F, class A>
std::ostream& operator << (std::ostream& out, boost::numeric::ublas::tensor<V,F,A> const& t)
{

	if(t.extents().is_scalar()){
		out << '[';
		boost::numeric::ublas::detail::print(out,t[0]);
		out << ']';
	}
	else if(t.extents().is_vector()) {
		const auto& cat = t.extents().at(0) > t.extents().at(1) ? ';' : ',';
		out << '[';
		for(auto i = 0u; i < t.size()-1; ++i){
			boost::numeric::ublas::detail::print(out,t[i]);
			out << cat << ' ';
		}
		boost::numeric::ublas::detail::print(out,t[t.size()-1]);
		out << ']';
	}
	else{
		boost::numeric::ublas::detail::print(out, t.rank()-1, t.data(), t.strides().data(), t.extents().data());
	}
	return out;
}


#endif
