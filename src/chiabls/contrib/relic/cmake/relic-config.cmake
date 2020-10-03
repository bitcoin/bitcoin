find_path(RELIC_INCLUDE_DIR relic/relic.h)
find_library(RELIC_LIBRARY NAMES relic)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(RELIC DEFAULT_MSG RELIC_INCLUDE_DIR RELIC_LIBRARY)

if(RELIC_FOUND)
	set(RELIC_LIBRARIES ${RELIC_LIBRARY})
	set(RELIC_INCLUDE_DIRS ${RELIC_INCLUDE_DIR})
endif()
