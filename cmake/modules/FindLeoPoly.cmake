#
#  FindLeoPoly.cmake
#
#  Created on 2016-11-6 by Seth Alves
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# LEOPOLY_FOUND               - LeoPoly library was found
# LEOPOLY_INCLUDE_DIR         - Path to LeoPoly include dir
# LEOPOLY_INCLUDE_DIRS        - Path to LeoPoly and zlib include dir (combined from LEOPOLY_INCLUDE_DIR + ZLIB_INCLUDE_DIR)
# LEOPOLY_LIBRARIES           - List of LeoPoly libraries
# LEOPOLY_ZLIB_INCLUDE_DIR    - The include dir of zlib headers

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("leopoly")

if (WIN32)
    find_path(LEOPOLY_INCLUDE_DIRS leopoly.h PATH_SUFFIXES include/leopoly HINTS ${LEOPOLY_SEARCH_DIRS})
elseif (APPLE)
    find_path(LEOPOLY_INCLUDE_DIRS leopoly.h PATH_SUFFIXES include/leopoly HINTS ${LEOPOLY_SEARCH_DIRS})
else ()
    find_path(LEOPOLY_INCLUDE_DIRS leopoly.h PATH_SUFFIXES leopoly HINTS ${LEOPOLY_SEARCH_DIRS})
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LEOPOLY DEFAULT_MSG LEOPOLY_INCLUDE_DIRS)

mark_as_advanced(LEOPOLY_INCLUDE_DIRS LEOPOLY_SEARCH_DIRS)
