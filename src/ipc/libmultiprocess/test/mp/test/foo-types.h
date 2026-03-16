// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_FOO_TYPES_H
#define MP_TEST_FOO_TYPES_H

#include <mp/proxy.h>
#include <mp/proxy-types.h>

// IWYU pragma: begin_exports
#include <capnp/common.h>
#include <cstddef>
#include <mp/test/foo.capnp.h>
#include <mp/type-context.h>
#include <mp/type-decay.h>
#include <mp/type-function.h>
#include <mp/type-interface.h>
#include <mp/type-map.h>
#include <mp/type-message.h>
#include <mp/type-number.h>
#include <mp/type-set.h>
#include <mp/type-string.h>
#include <mp/type-struct.h>
#include <mp/type-threadmap.h>
#include <mp/type-vector.h>
#include <string>
#include <type_traits>
// IWYU pragma: end_exports

namespace mp {
namespace test {
namespace messages {
struct ExtendedCallback; // IWYU pragma: export
struct FooCallback; // IWYU pragma: export
struct FooFn; // IWYU pragma: export
struct FooInterface; // IWYU pragma: export
} // namespace messages

template <typename Output>
void CustomBuildField(TypeList<FooCustom>, Priority<1>, InvokeContext& invoke_context, const FooCustom& value, Output&& output)
{
    BuildField(TypeList<std::string>(), invoke_context, output, value.v1);
    output.setV2(value.v2);
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<FooCustom>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    messages::FooCustom::Reader custom = input.get();
    return read_dest.update([&](FooCustom& value) {
        value.v1 = ReadField(TypeList<std::string>(), invoke_context, mp::Make<mp::ValueField>(custom.getV1()), ReadDestTemp<std::string>());
        value.v2 = custom.getV2();
    });
}

} // namespace test

inline void CustomBuildMessage(InvokeContext& invoke_context,
                        const test::FooMessage& src,
                        test::messages::FooMessage::Builder&& builder)
{
    builder.setMessage(src.message + " build");
}

inline void CustomReadMessage(InvokeContext& invoke_context,
                       const test::messages::FooMessage::Reader& reader,
                       test::FooMessage& dest)
{
    dest.message = std::string{reader.getMessage()} + " read";
}

inline void CustomBuildMessage(InvokeContext& invoke_context,
                        const test::FooMutable& src,
                        test::messages::FooMutable::Builder&& builder)
{
    builder.setMessage(src.message + " build");
}

inline void CustomReadMessage(InvokeContext& invoke_context,
                       const test::messages::FooMutable::Reader& reader,
                       test::FooMutable& dest)
{
    dest.message = std::string{reader.getMessage()} + " read";
}

inline void CustomPassMessage(InvokeContext& invoke_context,
                       const test::messages::FooMutable::Reader& reader,
                       test::messages::FooMutable::Builder builder,
                       std::function<void(test::FooMutable&)>&& fn)
{
    test::FooMutable mut;
    mut.message = std::string{reader.getMessage()} + " pass";
    fn(mut);
    builder.setMessage(mut.message + " return");
}
} // namespace mp

#endif // MP_TEST_FOO_TYPES_H
