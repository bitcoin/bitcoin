#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <algorithm>

class VectorUtil
{
public:
    template<class T>
    static std::vector<T*> ToPointerVec(std::vector<T>& in)
    {
        std::vector<T*> out;
        out.reserve(in.size());
        std::transform(
            in.begin(), in.end(),
            std::back_inserter(out),
            [](T& value) { return &value; }
        );

        return out;
    }
};