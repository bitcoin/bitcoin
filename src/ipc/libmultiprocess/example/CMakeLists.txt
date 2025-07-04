# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(${PROJECT_SOURCE_DIR}/cmake/TargetCapnpSources.cmake)

add_executable(mpcalculator
  calculator.cpp
)
target_capnp_sources(mpcalculator ${CMAKE_CURRENT_SOURCE_DIR} init.capnp calculator.capnp printer.capnp)
target_include_directories(mpcalculator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mpcalculator PRIVATE Threads::Threads)

add_executable(mpprinter
  printer.cpp
)
target_capnp_sources(mpprinter ${CMAKE_CURRENT_SOURCE_DIR} init.capnp calculator.capnp printer.capnp)
target_include_directories(mpprinter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mpprinter PRIVATE Threads::Threads)

add_executable(mpexample
  example.cpp
)
target_capnp_sources(mpexample ${CMAKE_CURRENT_SOURCE_DIR} init.capnp calculator.capnp printer.capnp)
target_include_directories(mpexample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mpexample PRIVATE Threads::Threads)
target_link_libraries(mpexample PRIVATE stdc++fs)

add_custom_target(mpexamples DEPENDS mpexample mpcalculator mpprinter)
