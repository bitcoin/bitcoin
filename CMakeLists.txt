cmake_minimum_required(VERSION 3.3)
project(ravencoin)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(build-ravencoin ALL
        COMMAND ./autogen.sh
        COMMAND ./configure
        COMMAND $(MAKE) WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND $(MAKE) install)

add_custom_target(debug-ravencoind ALL)
add_custom_target(debug-ravencoin-qt ALL)
add_custom_target(debug-test_raven ALL)
