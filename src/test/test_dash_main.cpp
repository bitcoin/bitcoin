// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Dash Test Suite

#include <banman.h>
#include <net.h>
#include <stacktraces.h>

#include <memory>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>

std::unique_ptr<CConnman> g_connman;
std::unique_ptr<BanMan> g_banman;

[[noreturn]] void Shutdown(void* parg)
{
  std::exit(EXIT_SUCCESS);
}

[[noreturn]] void StartShutdown()
{
  std::exit(EXIT_SUCCESS);
}

bool ShutdownRequested()
{
  return false;
}

template<typename T>
void translate_exception(const T &e)
{
    std::cerr << GetPrettyExceptionStr(std::current_exception()) << std::endl;
    throw;
}

template<typename T>
void register_exception_translator()
{
    boost::unit_test::unit_test_monitor.register_exception_translator<T>(&translate_exception<T>);
}

struct ExceptionInitializer {
    ExceptionInitializer()
    {
        RegisterPrettyTerminateHander();
        RegisterPrettySignalHandlers();

        register_exception_translator<std::exception>();
        register_exception_translator<std::string>();
        register_exception_translator<const char*>();
    }
    ~ExceptionInitializer()
    {
    }
};

BOOST_GLOBAL_FIXTURE(ExceptionInitializer);
