// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_FOO_H
#define MP_TEST_FOO_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <set>
#include <unordered_set>
#include <vector>

namespace mp {
namespace test {

//! Hash function used to test std::unordered_set with a custom hash.
struct FooHash
{
    size_t operator()(const std::string& str) const { return str.size(); }
};

struct FooStruct
{
    std::string name;
    std::set<int> set_int;
    std::vector<bool> vector_bool;
    std::optional<int> optional_int;
    std::unordered_set<int> unordered_set_int;
    std::map<std::string, int> map_string_int;
    // Containers with non-default comparator, allocator, and hash types.
    std::set<int, std::greater<>> set_int_desc;
    std::pmr::vector<int> pmr_vector_int;
    std::unordered_set<std::string, FooHash> unordered_set_string_hash;
    std::map<std::string, int, std::greater<>> map_string_int_desc;
};

enum class FooEnum : uint8_t { ONE = 1, TWO = 2, };

struct FooCustom
{
    std::string v1;
    int v2;
    std::vector<int> v3;
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

using FooData = std::vector<char>;
using FooDataRef = std::shared_ptr<const FooData>;

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

class FooInit
{
};

//! A second, arbitrary interface used to test returning a
//! list of interface objects.
class Bar
{
public:
    virtual ~Bar() = default;
    virtual int value() = 0;
};

//! Concrete Bar that returns a fixed value, used by listBars tests.
class SimpleBar : public Bar
{
public:
    explicit SimpleBar(int value) : m_value(value) {}
    int value() override { return m_value; }
    int m_value;
};
class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    void addOut(int a, int b, int& out) { out = a + b; }
    void addInOut(int x, int& sum) { sum += x; }
    int mapSize(const std::map<std::string, std::string>& map) { return map.size(); }
    FooStruct pass(FooStruct foo) { return foo; }
    [[noreturn]] void raise(FooStruct foo) { throw foo; }
    void initThreadMap() {}
    int callback(FooCallback& callback, int arg) { return callback.call(arg); }
    int callbackUnique(std::unique_ptr<FooCallback> callback, int arg) { return callback->call(arg); }
    int callbackShared(std::shared_ptr<FooCallback> callback, int arg) { return callback->call(arg); } // NOLINT(performance-unnecessary-value-param)
    void saveCallback(std::shared_ptr<FooCallback> callback) { m_callback = std::move(callback); }
    int callbackSaved(int arg) { return m_callback->call(arg); }
    int callbackExtended(ExtendedCallback& callback, int arg) { return callback.callExtended(arg); }
    FooCustom passCustom(FooCustom foo) { return foo; }
    FooEmpty passEmpty(FooEmpty foo) { return foo; }
    FooData passData(FooData foo) { return foo; }
    FooMessage passMessage(FooMessage foo) { foo.message += " call"; return foo; }
    void passMutable(FooMutable& foo) { foo.message += " call"; }
    FooEnum passEnum(FooEnum foo) { return foo; }
    double passDouble(double value) { return value; }
    int passFn(std::function<int()> fn) { return fn(); }
    std::vector<FooDataRef> passDataPointers(std::vector<FooDataRef> values) { return values; }
    std::vector<std::unique_ptr<Bar>> listBars(int n)
    {
        std::vector<std::unique_ptr<Bar>> result;
        result.reserve(n);
        for (int i = 0; i < n; ++i) result.push_back(std::make_unique<SimpleBar>(i));
        return result;
    }
    std::shared_ptr<FooCallback> m_callback;
    void callFn() { assert(m_fn); m_fn(); }
    void callFnAsync() { assert(m_fn); m_fn(); }
    int callIntFnAsync(int arg) { assert(m_int_fn); return m_int_fn(arg); }
    FooMessage callMessageAsync() { assert(m_fn); m_fn(); return {}; }
    std::function<void()> m_fn;
    std::function<int(int)> m_int_fn;
};

} // namespace test
} // namespace mp

#endif // MP_TEST_FOO_H
