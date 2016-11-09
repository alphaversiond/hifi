#
#  FindLeoPoly.cmake
#
#  Created on 2016-11-6 by Seth Alves
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# LEOPOLY_FOUND           - LeoPoly library was found
# LEOPOLY_INCLUDE_DIR     - Path to LeoPoly include dir
# LEOPOLY_INCLUDE_DIRS    - Path to LeoPoly and zlib include dir (combined from LEOPOLY_INCLUDE_DIR + ZLIB_INCLUDE_DIR)
# LEOPOLY_LIBRARIES       - List of LeoPoly libraries

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("leopoly")

if (WIN32)
  find_path(LEOPOLY_INCLUDE_DIRS Plugin.h HINTS ${LEOPOLY_SEARCH_DIRS})
  find_path(LEOPOLY_DLL_PATH SculptAppDLL.dll HINTS ${LEOPOLY_SEARCH_DIRS})
elseif (APPLE)
  find_path(LEOPOLY_INCLUDE_DIRS Plugin.h HINTS ${LEOPOLY_SEARCH_DIRS})
else ()
  find_path(LEOPOLY_INCLUDE_DIRS Plugin.h HINTS ${LEOPOLY_SEARCH_DIRS})
endif ()

include(FindPackageHandleStandardArgs)

if (WIN32)
  find_package_handle_standard_args(LEOPOLY DEFAULT_MSG LEOPOLY_INCLUDE_DIRS LEOPOLY_DLL_PATH)
  add_paths_to_fixup_libs("${LEOPOLY_DLL_PATH}")
else ()
  find_package_handle_standard_args(LEOPOLY DEFAULT_MSG LEOPOLY_INCLUDE_DIRS)
endif ()

mark_as_advanced(LEOPOLY_INCLUDE_DIRS LEOPOLY_SEARCH_DIRS)
