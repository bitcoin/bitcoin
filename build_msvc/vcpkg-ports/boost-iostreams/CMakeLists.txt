cmake_minimum_required(VERSION 3.5)
project(boost-iostreams)

if(BUILD_SHARED_LIBS)
    add_definitions(-DBOOST_IOSTREAMS_DYN_LINK=1)
endif()
add_definitions(
    -DBOOST_ALL_NO_LIB=1 
    -DBOOST_IOSTREAMS_USE_DEPRECATED
)
include_directories(include)
file(GLOB SOURCES src/*.cpp)
add_library(boost_iostreams ${SOURCES})

if(MSVC)
    set_property(TARGET boost_iostreams PROPERTY OUTPUT_NAME_RELEASE boost_iostreams-vc140-mt)
    set_property(TARGET boost_iostreams PROPERTY OUTPUT_NAME_DEBUG boost_iostreams-vc140-mt-gd)

    set_property(TARGET boost_iostreams PROPERTY RUNTIME_OUTPUT_NAME_RELEASE boost_iostreams)
    set_property(TARGET boost_iostreams PROPERTY RUNTIME_OUTPUT_NAME_DEBUG boost_iostreamsd)
endif()

find_package(ZLIB REQUIRED)
target_link_libraries(boost_iostreams PRIVATE ZLIB::ZLIB)

find_package(BZip2 REQUIRED)
target_link_libraries(boost_iostreams PRIVATE BZip2::BZip2)

find_package(LibLZMA REQUIRED)
target_include_directories(boost_iostreams PRIVATE ${LIBLZMA_INCLUDE_DIRS})
target_link_libraries(boost_iostreams PRIVATE ${LIBLZMA_LIBRARIES})

find_library(ZSTD_LIBRARY NAMES zstdd zstd_staticd zstd zstd_static NAMES_PER_DIR)
target_link_libraries(boost_iostreams PRIVATE ${ZSTD_LIBRARY})

install(TARGETS boost_iostreams
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
