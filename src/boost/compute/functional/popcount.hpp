//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_POPCOUNT_HPP
#define BOOST_COMPUTE_FUNCTIONAL_POPCOUNT_HPP

#include <boost/compute/function.hpp>
#include <boost/compute/type_traits/type_name.hpp>

namespace boost {
namespace compute {

/// Returns the number of non-zero bits in \p x.
///
/// \see_opencl_ref{popcount}
template<class T>
class popcount : public function<T(T)>
{
public:
    popcount()
        : function<T(T)>("boost_popcount")
    {
        std::stringstream s;
        s << "inline " << type_name<T>() << " boost_popcount"
          << "(const " << type_name<T>() << " x)\n"
          << "{\n"
          // use built-in popcount if opencl 1.2 is supported
          << "#if __OPENCL_VERSION__ >= 120\n"
          << "    return popcount(x);\n"
          // fallback to generic popcount() implementation
          << "#else\n"
          << "    " << type_name<T>() << " count = 0;\n"
          << "    for(" << type_name<T>() << " i = 0; i < sizeof(i) * CHAR_BIT; i++){\n"
          << "        if(x & (" << type_name<T>() << ") 1 << i){\n"
          << "            count++;\n"
          << "        }\n"
          << "    }\n"
          << "    return count;\n"
          << "#endif\n"
          << "}\n";
        this->set_source(s.str());
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_POPCOUNT_HPP
