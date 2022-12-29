set(TEST_FP "${CMAKE_BINARY_DIR}/bin/test_fp")
set(FIAT_TXT "${CMAKE_BINARY_DIR}/test_fp.txt")
set(FIAT_LOW "${CMAKE_SOURCE_DIR}/src/low/fiat/")
set(FIAT_FP "${FIAT_LOW}/fiat_fp.c")
set(MONT "src/ExtractionOCaml/word_by_word_montgomery")

message(STATUS "Running test_fp to discover prime modulus.")
execute_process(COMMAND ${TEST_FP} OUTPUT_FILE ${FIAT_TXT})
file(READ ${FIAT_TXT} OUTPUT_CONTENT)
string(REGEX MATCHALL "[(0-9)|(A-F)]+[ \n]" MATCHES ${OUTPUT_CONTENT})

set(LONGEST "0")
foreach(MATCH ${MATCHES})
	STRING(STRIP "${MATCH}" MATCH)
	STRING(LENGTH "${MATCH}" LEN)
	if (${LEN} GREATER_EQUAL ${LONGEST})
		set(LONGEST ${LEN})
	endif()
endforeach()

math(EXPR WSIZE "4 * ${LONGEST}")

foreach(MATCH ${MATCHES})
	STRING(STRIP "${MATCH}" MATCH)
	STRING(LENGTH "${MATCH}" LEN)
	if (${LEN} EQUAL ${LONGEST})
		set(PRIME "${PRIME}${MATCH}")
	endif()
endforeach()

execute_process(COMMAND $ENV{FIAT_CRYPTO}/${MONT} fp 64 "0x${PRIME}" OUTPUT_FILE ${FIAT_FP})

file(READ ${FIAT_FP} OUTPUT_CONTENT)
string(REPLACE "void" "static void" OUTPUT_FIXED "${OUTPUT_CONTENT}")
file(WRITE ${FIAT_FP} "${OUTPUT_FIXED}")

configure_file(${FIAT_LOW}/relic_fp_add_low.tmpl ${FIAT_LOW}/relic_fp_add_low.c COPYONLY)
configure_file(${FIAT_LOW}/relic_fp_mul_low.tmpl ${FIAT_LOW}/relic_fp_mul_low.c COPYONLY)
configure_file(${FIAT_LOW}/relic_fp_sqr_low.tmpl ${FIAT_LOW}/relic_fp_sqr_low.c COPYONLY)
