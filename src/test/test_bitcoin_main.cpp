// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/MIT.

#define BOOST_TEST_MODULE Bitcoin Test Suite

#include <net.h>

#include <boost/test/unit_test.hpp>

std::unique_ptr<CConnman> g_connman;

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
