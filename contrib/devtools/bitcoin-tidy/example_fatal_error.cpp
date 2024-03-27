// Copyright (c) 2024-present Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util/result.h"

#include <functional>
#include <string>
#include <thread>

// Test for bitcoin-fatal-error

using util::Result;

enum class FatalError
{
    Fatal
};

enum class ChainstateLoadError
{
    Error
};

Result<void, FatalError> void_fatal_func()
{
    return {};
}

Result<bool, FatalError> bool_fatal_func()
{
    return true;
}

Result<int, FatalError> int_fatal_func()
{
    return 1;
}

class Test {
public:
    Result<void, FatalError> fatal_func() {
        return void_fatal_func();
    }
};

template<typename T>
void CheckFatalBad(Result<T, FatalError> fatal, int i, int j) {
    return;
}

template<typename T>
void CheckFatal(Result<T, FatalError> fatal, int i, int j) {
    return;
}

void TraceThread(std::string name, std::function<void()> thread_func) {
    thread_func();
}

Result<void, FatalError> good_fatal_func_1()
{
    auto lambda_1 = ([&]() {
        return void_fatal_func();
    });

    auto lambda_2 = ([&]() {
        Result<void, FatalError> result{};
        result.Set(void_fatal_func());
    });

    auto lambda_3 = ([&]() {
        Result<void, FatalError> result{};
        result.MoveMessages(void_fatal_func());
    });

    auto lambda_4 = ([&]() {
        Result<void, FatalError> result{};
        auto res{void_fatal_func()};
        result.MoveMessages(res);
    });
    return {};
}

Result<void, FatalError> good_fatal_func_2()
{
    Result<void, FatalError> result{};
    auto res{void_fatal_func()};
    result.MoveMessages(res);
    return result;
}

Result<void, FatalError> good_fatal_func_3()
{
    auto res{void_fatal_func()};
    return res;
}

Result<void, FatalError> good_fatal_func_4()
{
    Result<void, FatalError> result{};
    result.Set(void_fatal_func());
    result.MoveMessages(void_fatal_func());
    if (result)
    {
        return void_fatal_func();
    }
    return result;
}

Result<void, FatalError> good_fatal_func_5()
{
    Result<void, FatalError> result{};
    if (auto res{void_fatal_func()}; !res)
    {
        result.MoveMessages(res);
        return result;
    }
    return {};
}

Result<int, FatalError> good_fatal_func_6()
{
    Test testy{};
    auto res{testy.fatal_func()};
    Result<void, FatalError> result{};
    result.MoveMessages(res);
    return 0;
}

Result<int, FatalError> good_fatal_func_7()
{
    Result<int, FatalError> result{1};
    result.Set([&]()
    {
        return int_fatal_func();
    }());
    return int_fatal_func();
}

Result<void, FatalError> good_fatal_func_8()
{
    Result<void, FatalError> result{};
    result.Set(void_fatal_func());
    auto res{void_fatal_func()};
    if (!res) {
        result.Set(std::move(res));
    } else {
        result.MoveMessages(res);
    }
    return result;
}

void good_fatal_func_9()
{
    CheckFatal(bool_fatal_func(), 0, 0);
}

void good_fatal_func_10()
{
    [&]()
    {
        CheckFatal(void_fatal_func(), 0, 0);
    }();
}

int good_fatal_func_11()
{
    int i = 0;
    if (1) {
        i += 1;
    }
    auto handle = std::thread(&TraceThread, "", [] {
        CheckFatal(void_fatal_func(), 0, 0);
    });
    return i;
}

Result<int, ChainstateLoadError> good_fatal_func_12()
{
    bool_fatal_func();
    return {0};
}

void bad_fatal_func_1()
{
    void_fatal_func();
    auto testy{void_fatal_func()};
}

void bad_fatal_func_2()
{
    Test testy{};
    testy.fatal_func();
}

void bad_fatal_func_3()
{
    [&]()
    {
        void_fatal_func();
    }();
}

void bad_fatal_func_4()
{
    CheckFatalBad(void_fatal_func(), 0, 0);
}

void bad_fatal_func_5()
{
    CheckFatal(void_fatal_func(), 0, 0);
    void_fatal_func();
}

void bad_fatal_func_6()
{
    int zero{0};
    void_fatal_func();
    CheckFatal(void_fatal_func(), 0, 0);
}

Result<int, FatalError> bad_fatal_func_7()
{
    [&]()
    {
        return void_fatal_func();
    }();
    return {0};
}

Result<int, FatalError> bad_fatal_func_8()
{
    auto res{void_fatal_func()};
    if (!res) return {util::Error{}, util::MoveMessages(res), res.GetFailure()};
    return 0;
}
