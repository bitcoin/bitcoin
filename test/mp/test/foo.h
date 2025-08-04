// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_FOO_H
#define MP_TEST_FOO_H

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <vector>

namespace mp {
namespace test {

struct FooStruct
{
    std::string name;
    std::set<int> setint;
    std::vector<bool> vbool;
};

enum class FooEnum : uint8_t { ONE = 1, TWO = 2, };

struct FooCustom
{
    std::string v1;
    int v2;
};

struct FooEmpty
{
};

struct FooMessage
{
    std::string message;
};

struct FooMutable
{
    std::string message;
};

class FooCallback
{
public:
    virtual ~FooCallback() = default;
    virtual int call(int arg) = 0;
};

class ExtendedCallback : public FooCallback
{
public:
    virtual int callExtended(int arg) = 0;
};

class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    int mapSize(const std::map<std::string, std::string>& map) { return map.size(); }
    FooStruct pass(FooStruct foo) { return foo; }
    void raise(FooStruct foo) { throw foo; }
    void initThreadMap() {}
    int callback(FooCallback& callback, int arg) { return callback.call(arg); }
    int callbackUnique(std::unique_ptr<FooCallback> callback, int arg) { return callback->call(arg); }
    int callbackShared(std::shared_ptr<FooCallback> callback, int arg) { return callback->call(arg); } // NOLINT(performance-unnecessary-value-param)
    void saveCallback(std::shared_ptr<FooCallback> callback) { m_callback = std::move(callback); }
    int callbackSaved(int arg) { return m_callback->call(arg); }
    int callbackExtended(ExtendedCallback& callback, int arg) { return callback.callExtended(arg); }
    FooCustom passCustom(FooCustom foo) { return foo; }
    FooEmpty passEmpty(FooEmpty foo) { return foo; }
    FooMessage passMessage(FooMessage foo) { foo.message += " call"; return foo; }
    void passMutable(FooMutable& foo) { foo.message += " call"; }
    FooEnum passEnum(FooEnum foo) { return foo; }
    int passFn(std::function<int()> fn) { return fn(); }
    std::shared_ptr<FooCallback> m_callback;
    void callFn() { assert(m_fn); m_fn(); }
    void callFnAsync() { assert(m_fn); m_fn(); }
    std::function<void()> m_fn;
};

} // namespace test
} // namespace mp

#endif // MP_TEST_FOO_H
