# Finds the header-only RapidJSON library.
# Hints: RAPIDJSON_ROOT (directory containing include/rapidjson)
find_path(RAPIDJSON_INCLUDE_DIRS rapidjson/rapidjson.h
    HINTS ${RAPIDJSON_ROOT} $ENV{RAPIDJSON_ROOT}
    PATH_SUFFIXES include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON DEFAULT_MSG RAPIDJSON_INCLUDE_DIRS)
mark_as_advanced(RAPIDJSON_INCLUDE_DIRS)
