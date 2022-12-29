find_path(RLC_INCLUDE_DIR relic/relic.h)
find_library(RLC_LIBRARY NAMES relic)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(RELIC DEFAULT_MSG RLC_INCLUDE_DIR RLC_LIBRARY)

if(RLC_FOUND)
	set(RLC_LIBRARIES ${RLC_LIBRARY})
	set(RLC_INCLUDE_DIRS ${RLC_INCLUDE_DIR})
endif()
