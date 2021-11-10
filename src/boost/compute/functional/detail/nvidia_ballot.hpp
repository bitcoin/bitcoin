//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_DETAIL_NVIDIA_BALLOT_HPP
#define BOOST_COMPUTE_FUNCTIONAL_DETAIL_NVIDIA_BALLOT_HPP

#include <boost/compute/function.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
class nvidia_ballot : public function<uint_(T)>
{
public:
    nvidia_ballot()
        : function<uint_(T)>("nvidia_ballot")
    {
        this->set_source(
            "inline uint nvidia_ballot(const uint x)\n"
            "{\n"
            "    uint result;\n"
            "    asm volatile(\n"
            "        \"setp.ne.u32 %%p1, %1, 0;\"\n"
            "        \"vote.ballot.b32 %0, %%p1;\"\n"
            "        : \"=r\"(result)\n"
            "        : \"r\"(x)\n"
            "    );\n"
            "    return result;\n"
            "}\n"
        );
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_DETAIL_NVIDIA_BALLOT_HPP
