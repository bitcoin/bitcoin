# - All variables are namespaced with UNIVALUE_ to avoid colliding with
#     downstream makefiles.
# - All Variables ending in _HEADERS or _SOURCES confuse automake, so the
#     _INT postfix is applied.
# - Convenience variables, for example a UNIVALUE_TEST_DIR should not be used
#     as they interfere with automatic dependency generation
# - The %reldir% is the relative path from the Makefile.am. This allows
#   downstreams to use these variables without having to manually account for
#   the path change.

UNIVALUE_INCLUDE_DIR_INT = %reldir%/include

UNIVALUE_DIST_HEADERS_INT =
UNIVALUE_DIST_HEADERS_INT += %reldir%/include/univalue.h

UNIVALUE_LIB_HEADERS_INT =
UNIVALUE_LIB_HEADERS_INT += %reldir%/lib/univalue_utffilter.h
UNIVALUE_LIB_HEADERS_INT += %reldir%/lib/univalue_escapes.h

UNIVALUE_LIB_SOURCES_INT =
UNIVALUE_LIB_SOURCES_INT += %reldir%/lib/univalue.cpp
UNIVALUE_LIB_SOURCES_INT += %reldir%/lib/univalue_get.cpp
UNIVALUE_LIB_SOURCES_INT += %reldir%/lib/univalue_read.cpp
UNIVALUE_LIB_SOURCES_INT += %reldir%/lib/univalue_write.cpp

UNIVALUE_TEST_DATA_DIR_INT = %reldir%/test

UNIVALUE_TEST_UNITESTER_INT =
UNIVALUE_TEST_UNITESTER_INT += %reldir%/test/unitester.cpp

UNIVALUE_TEST_JSON_INT =
UNIVALUE_TEST_JSON_INT += %reldir%/test/test_json.cpp

UNIVALUE_TEST_NO_NUL_INT =
UNIVALUE_TEST_NO_NUL_INT += %reldir%/test/no_nul.cpp

UNIVALUE_TEST_OBJECT_INT =
UNIVALUE_TEST_OBJECT_INT += %reldir%/test/object.cpp

UNIVALUE_TEST_FILES_INT =
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail1.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail2.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail3.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail4.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail5.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail6.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail7.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail8.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail9.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail10.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail11.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail12.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail13.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail14.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail15.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail16.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail17.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail18.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail19.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail20.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail21.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail22.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail23.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail24.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail25.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail26.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail27.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail28.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail29.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail30.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail31.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail32.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail33.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail34.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail35.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail36.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail37.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail38.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail39.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail40.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail41.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail42.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail44.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/fail45.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/pass1.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/pass2.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/pass3.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/pass4.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round1.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round2.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round3.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round4.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round5.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round6.json
UNIVALUE_TEST_FILES_INT += %reldir%/test/round7.json
