include(CheckSymbolExists)

message (STATUS "Searching for sasl/sasl.h")
find_path (
    SASL_INCLUDE_DIRS NAMES sasl/sasl.h
    PATHS /include /usr/include /usr/local/include /usr/share/include /opt/include c:/sasl/include
    DOC "Searching for sasl/sasl.h")

if (SASL_INCLUDE_DIRS)
    message (STATUS "  Found in ${SASL_INCLUDE_DIRS}")
else ()
    message (STATUS "  Not found (specify -DCMAKE_INCLUDE_PATH=C:/path/to/sasl/include for SASL support)")
endif ()

message (STATUS "Searching for libsasl2")
find_library(
    SASL_LIBS NAMES sasl2
    PATHS /usr/lib /lib /usr/local/lib /usr/share/lib /opt/lib /opt/share/lib /var/lib c:/sasl/lib
    DOC "Searching for libsasl2")

if (SASL_LIBS)
    message (STATUS "  Found ${SASL_LIBS}")
else ()
    message (STATUS "  Not found (specify -DCMAKE_LIBRARY_PATH=C:/path/to/sasl/lib for SASL support)")
endif ()

if (SASL_INCLUDE_DIRS AND SASL_LIBS)
    set (SASL_FOUND 1)

    check_symbol_exists (
        sasl_client_done
        ${SASL_INCLUDE_DIRS}/sasl/sasl.h
        MONGOC_HAVE_SASL_CLIENT_DONE)

    if (MONGOC_HAVE_SASL_CLIENT_DONE)
        set (MONGOC_HAVE_SASL_CLIENT_DONE 1)
    else ()
        set (MONGOC_HAVE_SASL_CLIENT_DONE 0)
    endif ()
else ()
    set (SASL_FOUND 0)
    set (MONGOC_HAVE_SASL_CLIENT_DONE 0)
endif ()
