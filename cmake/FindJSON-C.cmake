# cmake/FindJSON-C.cmake

# Guard against multiple inclusion which causes infinite recursion
if(FIND_JSON_C_INCLUDED)
    return()
endif()
set(FIND_JSON_C_INCLUDED TRUE)

find_path(JSON_C_INCLUDE_DIR json-c/json.h)
find_library(JSON_C_LIBRARY NAMES json-c)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSON-C DEFAULT_MSG
        JSON_C_LIBRARY JSON_C_INCLUDE_DIR)

if(JSON-C_FOUND AND NOT TARGET JSON-C::JSON-C)
    add_library(JSON-C::JSON-C UNKNOWN IMPORTED)
    set_target_properties(JSON-C::JSON-C PROPERTIES
            IMPORTED_LOCATION "${JSON_C_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JSON_C_INCLUDE_DIR}")
endif()

mark_as_advanced(JSON_C_INCLUDE_DIR JSON_C_LIBRARY)