
##################################################################################################################
# ToolchainSetup - Toolchain parameters setup, useful for libraries and external libraries
# 
# TOOLCHAIN_STL POSSIBLE VALUES: gnustl (GNU STL -> gnustl_shared) | llvmstl (LLVM libc++ -> c++_shared) | default gnustl



##################################################################################################################

set(TOOLCHAINSETUP_CMAKE_TOOLCHAIN_FILE       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/android.toolchain.cmake")
set(TOOLCHAINSETUP_ANDROID_NATIVE_API_LEVEL   24)
set(TOOLCHAINSETUP_ANDROID_NDK_BAK            "$ENV{ANDROID_NDK}")
set(TOOLCHAINSETUP_ANDROID_NDK                "") # turn it off so it chooses the standalone one
set(TOOLCHAINSETUP_ANDROID_COMPILER_IS_CLANG  1)
set(TOOLCHAINSETUP_ANDROID_STL_GNU                "gnustl_shared")
set(TOOLCHAINSETUP_ANDROID_STL_LIBC               "c++_shared")

##################################################################################################################
# check_standalone_toolchain - returns the standalone toolchain path or (fatal) exits if not found
macro(check_standalone_toolchain __ANDROID_STANDALONE_TOOLCHAIN _ANDROID_NDK TOOLCHAIN_STL)
  message ( STATUS "ndk received ${_ANDROID_NDK}")

  if (TOOLCHAIN_STL STREQUAL "llvmstl")
    set(MY_TS_TOOLCHAIN_NAME "my-tc-24-libc")
    set(MY_TS_STL_PARAM "libc++")
  else()
    set(MY_TS_TOOLCHAIN_NAME "my-tc-24-gnu")
    set(MY_TS_STL_PARAM "gnustl")
  endif()

  set(___ANDROID_STANDALONE_TOOLCHAIN "${_ANDROID_NDK}/toolchains/${MY_TS_TOOLCHAIN_NAME}")
  if( NOT EXISTS "${___ANDROID_STANDALONE_TOOLCHAIN}" )
    message( FATAL_ERROR "The required standalone toolchain ${__ANDROID_STANDALONE_TOOLCHAIN} could not be found.
    Generate it with
      ${_ANDROID_NDK}/build/tools/make_standalone_toolchain.py --arch arm --api 24 --stl=${MY_TS_STL_PARAM} --install-dir ${ANDROID_NDK_BAK}/toolchains/${MY_TS_TOOLCHAIN_NAME}
    " )
  endif()
  set(${__ANDROID_STANDALONE_TOOLCHAIN} "${___ANDROID_STANDALONE_TOOLCHAIN}")
endmacro()

##################################################################################################################
# toolchain_setup - sets up the required variables for 
macro(toolchain_setup TOOLCHAIN_STL)
  message ( STATUS "toolchain_setup start")
  if (USE_ANDROID_TOOLCHAIN)

    if (TOOLCHAIN_STL STREQUAL "llvmstl")
      set(TOOLCHAINSETUP_ANDROID_STL "${TOOLCHAINSETUP_ANDROID_STL_LIBC}")
    else()
      set(TOOLCHAINSETUP_ANDROID_STL "${TOOLCHAINSETUP_ANDROID_STL_GNU}")
    endif()

    message ( STATUS "using android toolchain")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/android/android.toolchain.cmake")
    set(ANDROID_NATIVE_API_LEVEL 24)
    set(ANDROID_NDK_BAK "$ENV{ANDROID_NDK}")
    set(ANDROID_NDK "") # turn it off so it chooses the standalone one
    message ( STATUS "ndk ${ANDROID_NDK_BAK}")

    set(ANDROID_STANDALONE_TOOLCHAIN "")
    check_standalone_toolchain(ANDROID_STANDALONE_TOOLCHAIN ${ANDROID_NDK_BAK} ${TOOLCHAIN_STL})

    #set(__ANDROID_STANDALONE_TOOLCHAIN "${ANDROID_NDK_BAK}/toolchains/my-tc-24-libc")
    #if( NOT EXISTS "${__ANDROID_STANDALONE_TOOLCHAIN}" )
    #  message( FATAL_ERROR "The required standalone toolchain could not be found.
    #  Generate it with
    #    ${ANDROID_NDK_BAK}/build/tools/make_standalone_toolchain.py --arch arm --api 24 --stl=libc++ --install-dir ${ANDROID_NDK_BAK}/toolchains/my-tc-24-libc
    #  " )
    #endif()

    #set(ANDROID_STANDALONE_TOOLCHAIN "${__ANDROID_STANDALONE_TOOLCHAIN}")

    set(ANDROID_COMPILER_IS_CLANG 1)

    # the intention is still to use the shared library but for it only works
    # with the static one by now
    #set(ANDROID_STL c++_shared)
    set(ANDROID_STL "${TOOLCHAINSETUP_ANDROID_STL}")

    message ( STATUS "Value of native level... ${ANDROID_NATIVE_API_LEVEL} and toolchain ${ANDROID_STANDALONE_TOOLCHAIN}")
  else ()
    message ( STATUS "no android toolchain?")
  endif ()
  message ( STATUS "toolchain_setup end")
endmacro(toolchain_setup)



macro(toolchain_setup_args argsRet TOOLCHAIN_STL)
  message ( STATUS "toolchain_setup_args start")

  if (TOOLCHAIN_STL STREQUAL "llvmstl")
    set(TOOLCHAINSETUP_ANDROID_STL "${TOOLCHAINSETUP_ANDROID_STL_LIBC}")
  else()
    set(TOOLCHAINSETUP_ANDROID_STL "${TOOLCHAINSETUP_ANDROID_STL_GNU}")
  endif()

  set(xANDROID_STANDALONE_TOOLCHAIN "")
  check_standalone_toolchain(xANDROID_STANDALONE_TOOLCHAIN ${TOOLCHAINSETUP_ANDROID_NDK_BAK} ${TOOLCHAIN_STL})
  if (NOT args)
    list(APPEND args "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAINSETUP_CMAKE_TOOLCHAIN_FILE}")
    message ( STATUS "toolchain_setup_args middle ${args}")
    list(APPEND args "-DANDROID_NATIVE_API_LEVEL=${TOOLCHAINSETUP_ANDROID_NATIVE_API_LEVEL}")
    list(APPEND args "-DANDROID_STANDALONE_TOOLCHAIN=${xANDROID_STANDALONE_TOOLCHAIN}")
    list(APPEND args "-DANDROID_NDK=${TOOLCHAINSETUP_ANDROID_NDK}")
    list(APPEND args "-DANDROID_COMPILER_IS_CLANG=${TOOLCHAINSETUP_ANDROID_COMPILER_IS_CLANG}")
    list(APPEND args "-DANDROID_STL=${TOOLCHAINSETUP_ANDROID_STL}")
    # make the toolchain set c++11 as CMAKE_CXX_PARAM 
    list(APPEND args "-DSET_STD_CXX11=1")
  endif()

  list(APPEND ${argsRet} ${args})
  message ( STATUS "toolchain_setup_args end ${args} vs ${argsRet}")
  #set(args "")
endmacro(toolchain_setup_args)
