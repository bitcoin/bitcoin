// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_EXCEPTION_H
#define MP_PROXY_TYPE_EXCEPTION_H

#include <mp/util.h>

namespace mp {
template <typename Output>
void CustomBuildField(TypeList<std::exception>,
    Priority<1>,
    InvokeContext& invoke_context,
    const std::exception& value,
    Output&& output)
{
    BuildField(TypeList<std::string>(), invoke_context, output, std::string(value.what()));
}
} // namespace mp

#endif // MP_PROXY_TYPE_EXCEPTION_H
