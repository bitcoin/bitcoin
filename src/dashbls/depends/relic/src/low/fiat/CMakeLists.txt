set(INHERIT "gmp-sec")
include(../cmake/gmp.cmake)
if(GMP_FOUND)
	include_directories(${GMP_INCLUDE_DIR})
	set(ARITH_LIBS ${GMP_LIBRARIES})
endif(GMP_FOUND)

set(FIAT_CMAKE ${CMAKE_SOURCE_DIR}/cmake/fiat.cmake)
set(FIAT_FP "${CMAKE_SOURCE_DIR}/src/low/fiat/fiat_fp.c")

include(${FIAT_CMAKE})
add_custom_target(fiat DEPENDS ${FIAT_FP})
