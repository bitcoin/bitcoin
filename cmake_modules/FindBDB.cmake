# Locate Berkeley DB C++ 
# This module defines
# BDB_LIBRARY
# BDB_FOUND, if false, do not try to link to zlib 
# BDB_INCLUDE_DIR, where to find the headers
#
# $BDB_DIR is an environment variable that would
# correspond to the ./configure --prefix=$BDB_DIR
# used in building BDB.
#
# Created by Michael Gronager. 

find_path(BDB_INCLUDE_DIR db_cxx.h
    $ENV{BDB_DIR}/include
    $ENV{BDB_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /usr/local/opt/berkeley-db/include  # HomeBrew
    /usr/local/opt/berkeley-db4/include # HomeBrew, BDB4
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
)

find_library(BDB_LIBRARY 
    NAMES db_cxx libdb_cxx
    PATHS
    $ENV{BDB_DIR}/lib
    $ENV{BDB_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/lib
    /usr/lib
    /usr/local/opt/berkeley-db/lib  # HomeBrew
    /usr/local/opt/berkeley-db4/lib # HomeBrew, BDB4
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
)

set(BDB_FOUND "NO")
if(BDB_LIBRARY AND BDB_INCLUDE_DIR)
    set(BDB_FOUND "YES")
endif()

add_library(BDB::bdb UNKNOWN IMPORTED)
set_target_properties(BDB::bdb PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BDB_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${BDB_LIBRARY}")
 
