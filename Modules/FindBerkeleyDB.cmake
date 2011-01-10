# -*- cmake -*-

# - Find BerkeleyDB
# Find the BerkeleyDB includes and library
# This module defines
#  DB_INCLUDE_DIR, where to find db.h, etc.
#  DB_LIBRARIES, the libraries needed to use BerkeleyDB.
#  DB_FOUND, If false, do not try to use BerkeleyDB.
# also defined, but not for general use are
#  DB_LIBRARY, where to find the BerkeleyDB library.


FIND_PATH(BerkeleyDB_INCLUDE_DIR db_cxx.h
/usr/local/include/db4
/usr/local/include
/usr/include/db4
/usr/include
)

SET(BerkeleyDB_NAMES ${BerkeleyDB_NAMES} db db_cxx)


if (BerkeleyDB_FIND_VERSION_MAJOR AND BerkeleyDB_FIND_VERSION_MINOR)
  set(NAME_EXTENSION -${BerkeleyDB_FIND_VERSION_MAJOR}.${BerkeleyDB_FIND_VERSION_MINOR})
elseif (BerkeleyDB_FIND_VERSION_MAJOR)
  set(NAME_EXTENSION -${BerkeleyDB_FIND_VERSION_MAJOR})
endif(BerkeleyDB_FIND_VERSION_MAJOR AND BerkeleyDB_FIND_VERSION_MINOR)

if (NAME_EXTENSION)
  foreach (NAME ${BerkeleyDB_NAMES})
    set(NEW_NAMES ${NEW_NAMES} "${NAME}${NAME_EXTENSION}")
  endforeach (NAME)
  set (BerkeleyDB_NAMES ${NEW_NAMES})
endif (NAME_EXTENSION)


foreach(NAME ${BerkeleyDB_NAMES})
  FIND_LIBRARY(BerkeleyDB_LIBRARY${NAME}
    NAMES ${NAME}
    PATHS /usr/lib /usr/local/lib
    )
  
  SET(BerkeleyDB_LIBRARIES ${BerkeleyDB_LIBRARIES} ${BerkeleyDB_LIBRARY${NAME}})
endforeach(NAME)

# handle the QUIETLY and REQUIRED arguments and set JPEG_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BerkeleyDB DEFAULT_MSG BerkeleyDB_LIBRARIES BerkeleyDB_INCLUDE_DIR)


# Deprecated declarations.
SET (NATIVE_BerkeleyDB_INCLUDE_PATH ${BerkeleyDB_INCLUDE_DIR} )
#GET_FILENAME_COMPONENT (NATIVE_BerkeleyDB_LIB_PATH ${BerkeleyDB_LIBRARY} PATH)

MARK_AS_ADVANCED(
  BerkeleyDB_LIBRARIES
  BerkeleyDB_INCLUDE_DIR
  )
