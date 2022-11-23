# - Try to find c-rrb
# Once done, this will define
#
#  RRB_FOUND - system has RRB
#  RRB_INCLUDE_DIR - the RRB include directories
#  RRB_LIBRARIES - link these to use RRB

find_path(RRB_INCLUDE_DIR rrb.h)
find_library(RRB_LIBRARIES NAMES rrb librrb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  RRB DEFAULT_MSG RRB_LIBRARIES RRB_INCLUDE_DIR)

mark_as_advanced(RRB_INCLUDE_DIR RRB_LIBRARIES)
