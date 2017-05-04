# HiFi interface - Android build on Windows

----------------------------------------------------------------
## Prerequisites

  * Visual Studio 12.0 (Community is enough)
  * [Qt 5.6.1-1 android](http://download.qt.io/official_releases/qt/5.6/5.6.1-1/qt-opensource-windows-x86-android-5.6.1-1.exe.mirrorlist). Previously used 5.5!
  * ~~[Qt 5.6.1-1 for win](http://download.qt.io/official_releases/qt/5.6/5.6.1-1/qt-opensource-windows-x86-android-5.6.1-1.exe.mirrorlist)~~ Not needed for anything to build android
  * Android NDK r10e
  * ant 1.9.4
  * Android SDK Api level 21
  * Cmake 3.3.2
  * GNU Make for Windows

  ### check prerequisites
  With the utility checkhifi.bat you can check if you have the needed variables and programs.

  ### cl compiler 

  In case of cl not found, run vcvarsall.bat from your Visual Studio copy.
  ```
  "c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
  ```

  ----------------------------------------------------------------
  ## scribe
  HiFi tool for shader files

  ### Build independently from desktop Qt and Interface build

  Copy SetupHifiProject.cmake into the tools/scribe folder with these changes:
  ```diff
  $ git diff
  diff --git a/cmake/macros/SetupHifiProject.cmake b/cmake/macros/SetupHifiProject.cmake
  index 8695063..9d4bb4b 100644
  --- a/cmake/macros/SetupHifiProject.cmake
  +++ b/cmake/macros/SetupHifiProject.cmake
  @@ -27,11 +27,11 @@ macro(SETUP_HIFI_PROJECT)
     # include the generated application version header
     target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_BINARY_DIR}/includes")

  -  set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  -  list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core)
  +  #set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  +  #list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core)

     # find these Qt modules and link them to our own target
  -  find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)
  +  #find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)

     # disable /OPT:REF and /OPT:ICF for the Debug builds
     # This will prevent the following linker warnings
  @@ -40,9 +40,9 @@ macro(SETUP_HIFI_PROJECT)
        set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
     endif()

  -  foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
  -    target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
  -  endforeach()
  +  #foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
  +  #  target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
  +  #endforeach()

  -  target_glm()
  +  #target_glm()
  ```

  Modify the CMakeLists.txt file in tools/scribe:
  ```diff
  diff --git a/tools/scribe/CMakeLists.txt b/tools/scribe/CMakeLists.txt
  index e62a346..389cce7 100755
  --- a/tools/scribe/CMakeLists.txt
  +++ b/tools/scribe/CMakeLists.txt
  @@ -1,3 +1,5 @@
  -set(TARGET_NAME scribe)
  +cmake_minimum_required(VERSION 3.3)

  +set(TARGET_NAME scribe)
  +include("SetupHifiProject.cmake")
   setup_hifi_project()
  ```

  #### Build it!
  cd into scribe folder
  ```
  md build
  cd build
  cmake ..
  ```

  then
  ```
  MSBuild /nologo /t:Build scribe.vcxproj
  ```

  scribe/build/Debug/ should have scribe.exe

  #### Path and test
  update scribe path:
  ```
  set SCRIBE_PATH=c:\Users\user\dev\workspace-hifi\hifi\tools\scribe\build\Debug\
  setx SCRIBE_PATH %SCRIBE_PATH%
  ```
  test:
  ```
  %SCRIBE_PATH%/scribe
  ```
## Fix makefiles that use scribe
Generated makefiles use .. which is not working on Windows. So we must replace
  ```
  ../../../tools/scribe/build/Debug/scribe.exe
  ```
  for
  ```
  c:/Users/user/dev/workspace-hifi/hifi/tools/scribe/build/Debug/scribe.exe
  ```
  (Or in your case your full path to scribe)

### Use fixscribeinmakes.bat

Edit it, particularly to match paths in your system. Must check these variables inside:
```
FIX_SCRIBE_OLD
FIX_SCRIBE_NEW
```
And the line that changes dir to libraries should point your actual build-dir\libraries
```
cd  C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries
```

## Build
Use build_android.bat that runs cmake for you and creates a build folder.
Inside the build folder, run
```
make interface-apk
```

## ndk r12b + android-24 + Google VR

### prerequisites

download gvr
git clone https://github.com/googlevr/gvr-android-sdk
(where? in android_lib_dir location, generally one level outside the cloned hifi)

sdk:
android 7.0
sdk tools 25.2.2
sdk platform-tools 25

download r12b
https://github.com/android-ndk/ndk/wiki

python to make a standalone toolchain
https://www.python.org/ftp/python/2.7.12/python-2.7.12.msi

new openssl to replace old one
targeting android-24
(given a tar.gz, use
```
tar -zxvf backup.tar.gz
```
from git bash.

### create standalone toolchain
```
C:\Users\user\dev\bin\android-ndk-r12b\build\tools>make_standalone_toolchain.py --arch arm --api 24 --stl=libc++ --install-dir C:\Users\user\dev\bin\android-ndk-r12b\toolchains\my-tc-24-libc
```
  creates folder
  ```
  C:\Users\user\dev\bin\android-ndk-r12b\toolchains\my-tc-24-libc
  ```

Complete toolchain by copying libsupc
```
c:\Users\user\dev\bin\android-ndk-r12b\sources\cxx-stl\gnu-libstdc++\4.9\libs\armeabi-v7a\libsupc++.a
to
c:\Users\user\dev\bin\android-ndk-r12b\toolchains\my-tc-24-libc\arm-linux-androideabi\lib\armv7-a\
```
## Additional changes

### fix toolchain

Separate the suffix for clang from other tools (so some will use .exe, clang .cmd)

```
+++ b/cmake/android/android.toolchain.cmake
@@ -362,6 +362,7 @@ if( NOT DEFINED ANDROID_NDK_HOST_X64 AND (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "a
 endif()

 set( TOOL_OS_SUFFIX "" )
+set( TOOL_OS_CLANG_SCRIPT_SUFFIX "" )
 if( CMAKE_HOST_APPLE )
  set( ANDROID_NDK_HOST_SYSTEM_NAME "darwin-x86_64" )
  set( ANDROID_NDK_HOST_SYSTEM_NAME2 "darwin-x86" )
@@ -369,6 +370,7 @@ elseif( CMAKE_HOST_WIN32 )
  set( ANDROID_NDK_HOST_SYSTEM_NAME "windows-x86_64" )
  set( ANDROID_NDK_HOST_SYSTEM_NAME2 "windows" )
  set( TOOL_OS_SUFFIX ".exe" )
+ set( TOOL_OS_CLANG_SCRIPT_SUFFIX ".cmd" )
 elseif( CMAKE_HOST_UNIX )
  set( ANDROID_NDK_HOST_SYSTEM_NAME "linux-x86_64" )
  set( ANDROID_NDK_HOST_SYSTEM_NAME2 "linux-x86" )
@@ -1108,23 +1110,24 @@ else()
 endif()
 unset( _ndk_ccache )

-
+message (STATUS "CMAKE_C_COMPILER ${CMAKE_C_COMPILER}")
+message (STATUS "CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}")
 # setup the cross-compiler
 if( NOT CMAKE_C_COMPILER )
  if( NDK_CCACHE AND NOT ANDROID_SYSROOT MATCHES "[ ;\"]" )
   set( CMAKE_C_COMPILER   "${NDK_CCACHE}" CACHE PATH "ccache as C compiler" )
   set( CMAKE_CXX_COMPILER "${NDK_CCACHE}" CACHE PATH "ccache as C++ compiler" )
   if( ANDROID_COMPILER_IS_CLANG )
-   set( CMAKE_C_COMPILER_ARG1   "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}${TOOL_OS_SUFFIX}"   CACHE PATH "C compiler")
-   set( CMAKE_CXX_COMPILER_ARG1 "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}++${TOOL_OS_SUFFIX}" CACHE PATH "C++ compiler")
+   set( CMAKE_C_COMPILER_ARG1   "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}${TOOL_OS_CLANG_SCRIPT_SUFFIX}"   CACHE PATH "C compiler")
+   set( CMAKE_CXX_COMPILER_ARG1 "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}++${TOOL_OS_CLANG_SCRIPT_SUFFIX}" CACHE PATH "C++ compiler")
   else()
    set( CMAKE_C_COMPILER_ARG1   "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-gcc${TOOL_OS_SUFFIX}" CACHE PATH "C compiler")
    set( CMAKE_CXX_COMPILER_ARG1 "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-g++${TOOL_OS_SUFFIX}" CACHE PATH "C++ compiler")
   endif()
  else()
   if( ANDROID_COMPILER_IS_CLANG )
-   set( CMAKE_C_COMPILER   "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}${TOOL_OS_SUFFIX}"   CACHE PATH "C compiler")
-   set( CMAKE_CXX_COMPILER "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}++${TOOL_OS_SUFFIX}" CACHE PATH "C++ compiler")
+   set( CMAKE_C_COMPILER   "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}${TOOL_OS_CLANG_SCRIPT_SUFFIX}"   CACHE PATH "C compiler")
+   set( CMAKE_CXX_COMPILER "${ANDROID_CLANG_TOOLCHAIN_ROOT}/bin/${_clang_name}++${TOOL_OS_CLANG_SCRIPT_SUFFIX}" CACHE PATH "C++ compiler")
   else()
    set( CMAKE_C_COMPILER   "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-gcc${TOOL_OS_SUFFIX}"    CACHE PATH "C compiler" )
    set( CMAKE_CXX_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-g++${TOOL_OS_SUFFIX}"    CACHE PATH "C++ compiler" )
@@ -1144,7 +1147,8 @@ if( NOT CMAKE_C_COMPILER )
  set( CMAKE_OBJDUMP      "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump" )
  set( CMAKE_RANLIB       "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-ranlib${TOOL_OS_SUFFIX}"  CACHE PATH "ranlib" )
 endif()
-
+message (STATUS "CMAKE_C_COMPILER ${CMAKE_C_COMPILER}")
+message (STATUS "CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}")
 set( _CMAKE_TOOLCHAIN_PREFIX "${ANDROID_TOOLCHAIN_MACHINE_NAME}-" )
:
```
(USE .cmd for compilers)

### Troubleshooting

#### Path errors

At least ANDROID_LIB_DIR and NDK_PATH should use / slashes
```
ANDROID_LIB_DIR
C:\Users\user\dev\workspace-hifi\hifi>set ANDROID_LIB_DIR=c:/Users/user/dev/workspace-hifi/
C:\Users\user\dev\workspace-hifi\hifi>setx ANDROID_LIB_DIR %ANDROID_LIB_DIR%
```

#### Some    l i b w h a t v e r . s o    (e.g. like it being searching for wrong lib names - missing e) not found

Whenever the linker does not find a library, just provide the one with the name "suggested".

Example of the situation with a missing 's' in libprocedural.so (so it looks for libprocedural.o):
```
  [ 86%] Linking CXX shared library apk/libs/armeabi-v7a/libinterface.so
  clang++.exe: error: no such file or directory: 'apk/libs/armeabi-v7a/libprocedural.o'
  make[3]: *** [interface/apk/libs/armeabi-v7a/libinterface.so] Error 1
  make[2]: *** [interface/CMakeFiles/interface.dir/all] Error 2
  make[1]: *** [interface/CMakeFiles/interface-apk.dir/rule] Error 2
  make: *** [interface-apk] Error 2
```
Create a copy of the existing libprocedural.so file as *libprocedural.o* and then run make interface-apk again

**It is pending to discover why that happens, this is just a workaround to make it possible to build**

#### Syntax error when linking libinterface.so

Edit the build_dir/interface/CMakeFiles/interface.dir/build.make:
Look for the string
```
Linking CXX shared library apk/libs/armeabi-v7a/libinterface.so
```
Next to it there should be a call to *clang++.cmd*, replace it for **clang38++.exe**

#### build.xml error when running qtcreateapk

Apparently the code that runs "android update" on the gvr-common library doesn't run at all on Windows.
Manually run inside build-dir/interface/gvr-common:
```
%ANDROID_HOME%/tools/android update lib-project -p . -t android-24
```
(our current target is android-24)
Run interface-apk again inside build-dir.




## Appendix - Custom scripts
  We have some helpul and needed scripts to build Android on Windows as the process was outlined for OS X in the first place. Further work should make it possible to skip them.

  All these scripts should be accessible, by being in the PATH.

  checkhifi.bat - shows needed variables and executables
  ```
  @echo OFF

  ::call:checkVar ANDROID_HOME %ANDROID_HOME%

  echo:
  echo Checking environment variables...
  echo:
  SET VAR=ANDROID_HOME
  SET VAL=%ANDROID_HOME%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=ANDROID_NDK
  SET VAL=%ANDROID_NDK%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=NDK_HOME
  SET VAL=%NDK_HOME%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=ANDROID_NDK_ROOT
  SET VAL=%ANDROID_NDK_ROOT%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=ANDROID_LIB_DIR
  SET VAL=%ANDROID_LIB_DIR%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=QT_CMAKE_PREFIX_PATH
  SET VAL=%QT_CMAKE_PREFIX_PATH%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=QT_CMAKE_PREFIX_PATH_ANDROID
  SET VAL=%QT_CMAKE_PREFIX_PATH_ANDROID%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  SET VAR=SCRIBE_PATH
  SET VAL=%SCRIBE_PATH%
  if "%VAL%" == "" ( echo [ERROR] %VAR% Not defined ****** ) else ( echo [INFO ] %VAR% defined as [%VAL%] )
  echo:
  @echo Check finished...
  echo:
  echo:
  @echo Checking programs in path...
  echo:
  SET PROG=cl.exe
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" (
      echo [ERROR] %PROG% not found ******
      echo [INFO ] Must run "vcvarsall.bat" x64 from your "Visual Studio\VC" installation to find cl
  ) else (
      echo [INFO ] %PROG% found at "%FOUND%"
  )

  SET PROG=make.exe
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" ( echo [ERROR] %PROG% not found ****** ) else ( echo [INFO ] %PROG% found at "%FOUND%" )

  SET PROG=cp.bat
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" ( echo [ERROR] %PROG% not found ****** ) else ( echo [INFO ] %PROG% found at "%FOUND%" )

  SET PROG=cmake.exe
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" ( echo [ERROR] %PROG% not found ****** ) else ( echo [INFO ] %PROG% found at "%FOUND%" )

  SET PROG=java.exe
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" ( echo [ERROR] %PROG% not found ****** ) else ( echo [INFO ] %PROG% found at "%FOUND%" )

  SET PROG=javac.exe
  for %%X in (%PROG%) do (set FOUND=%%~$PATH:X)
  if "%FOUND%" == "" ( echo [ERROR] %PROG% not found ****** ) else ( echo [INFO ] %PROG% found at "%FOUND%" )

  goto:eof

  :eof
  ```

  cp.bat - "cp" for windows adaptation (make uses cp instead of copy)
  ```
  @echo off
  set a=%1
  set b=%2
  set a=%a:/=\%
  set b=%b:/=\%
  rem echo %a%
  rem echo %b%
  copy %a% %b%
  ```

  vars.bat (sets env vars automatically without having to go to the UI, edit first!)
  ```
  set ANDROID_HOME=c:/Users/user/dev/bin/android-sdk-windows/
  set ANDROID_NDK=c:\Users\user\dev\bin\android-ndk-r10e\
  set NDK_HOME=%ANDROID_NDK%
  set ANDROID_NDK_ROOT=%ANDROID_NDK%
  set ANDROID_LIB_DIR=c:\Users\user\dev\workspace-hifi\
  set QT_CMAKE_PREFIX_PATH=c:\Qt\Qt5.6.1\5.6\android_armv7\lib\cmake\
  set QT_CMAKE_PREFIX_PATH_ANDROID=c:\Qt\Qt5.6.1\5.6\android_armv7\lib\cmake\
  set SCRIBE_PATH=c:\Users\user\dev\workspace-hifi\hifi\tools\scribe\build\Debug\

  setx ANDROID_HOME %ANDROID_HOME%
  setx ANDROID_NDK %ANDROID_NDK%
  setx NDK_HOME %ANDROID_NDK%
  setx ANDROID_NDK_ROOT %ANDROID_NDK%
  setx ANDROID_LIB_DIR %ANDROID_LIB_DIR%
  setx QT_CMAKE_PREFIX_PATH %QT_CMAKE_PREFIX_PATH%
  setx SCRIBE_PATH %SCRIBE_PATH%

  :: Be sure to have the Path environment variable correctly set up
  :: With at least the following:
  ::
  :: JDK 8 
  :: e.g. c:\Program Files\Java\jdk1.8.0_101\bin
  ::
  :: ant
  :: C:\Users\user\dev\bin\apache-ant-1.9.4\bin
  ::
  :: cmake
  :: C:\Users\user\dev\bin\cmake-3.3.2-win32-x86\bin
  ::
  :: git 
  :: e.g. C:\Program Files\Git\cmd
  ::
  :: make for Windows
  :: c:\Program Files (x86)\GnuWin32\bin
  ::
  :: cp.bat
  :: c:\whereveryouputit
  ::
  ```

  replacerx.bat (replaces a string in a file and outputs) - **must be in PATH to make fixscribeinmakes.bat work**
  ```
  @echo off
  REM -- Prepare the Command Processor --
  SETLOCAL ENABLEEXTENSIONS
  SETLOCAL DISABLEDELAYEDEXPANSION

  ::BatchSubstitude - parses a File line by line and replaces a substring"
  ::syntax: BatchSubstitude.bat OldStr NewStr File
  ::          OldStr [in] - string to be replaced
  ::          NewStr [in] - string to replace with
  ::          File   [in] - file to be parsed
  :$changed 20100115
  :$source http://www.dostips.com
  if "%~1"=="" findstr "^::" "%~f0"&GOTO:EOF
  for /f "tokens=1,* delims=]" %%A in ('"type %3|find /n /v """') do (
      set "line=%%B"
      if defined line (
          call set "line=echo.%%line:%~1=%~2%%"
          for /f "delims=" %%X in ('"echo."%%line%%""') do %%~X
      ) ELSE echo.
  )
  ```

  fixscribeinmakes.bat (edit first! predefined files use .. and is not working on cmd)
  ```
  @echo off

  REM ****************************************
  REM 1 replacerx.bat in path

  REM ****************************************
  REM 2 locate all needed to change files:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\display-plugins\CMakeFiles\display-plugins.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\entities-renderer\CMakeFiles\entities-renderer.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\gpu\CMakeFiles\gpu.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\model\CMakeFiles\model.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\procedural\CMakeFiles\procedural.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\render\CMakeFiles\render.dir\build.make:
  REM C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries\render-utils\CMakeFiles\render-utils.dir\build.make:

  REM ****************************************
  REM 3 detect what to replace
  REM result:
  REM ../../../tools/scribe/build/Debug/scribe.exe
  SET FIX_SCRIBE_OLD=../../../tools/scribe/build/Debug/scribe.exe

  REM ****************************************
  REM 4 detect what for
  REM result:
  REM c:/Users/user/dev/workspace-hifi/hifi/tools/scribe/build/Debug/scribe.exe
  SET FIX_SCRIBE_NEW=c:/Users/user/dev/workspace-hifi/hifi/tools/scribe/build/Debug/scribe.exe

  REM ****************************************
  REM 5 go to libraries folder
  cd  C:\Users\user\dev\workspace-hifi\hifi\build-android-interface\libraries
  echo "in %cd%"

  REM ****************************************
  REM 6 run for each
  cd "display-plugins\CMakeFiles\display-plugins.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "entities-renderer\CMakeFiles\entities-renderer.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "gpu\CMakeFiles\gpu.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "model\CMakeFiles\model.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "procedural\CMakeFiles\procedural.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "render\CMakeFiles\render.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"

  REM ****************************************
  REM run for each
  cd "render-utils\CMakeFiles\render-utils.dir"
  echo "in %cd%"
  erase build.makexx
  call replacerx %FIX_SCRIBE_OLD% %FIX_SCRIBE_NEW% build.make > build.makexx
  erase build.make
  move build.makexx build.make
  cd ..\..\..
  echo "in %cd%"
  ```
  
build_android.bat (not needed in path, but it is recommended to be one level outside the cloned hifi folder) Edit it for correct folder paths!
```
:: Script to build android interface in windows development environment
@ECHO OFF
RMDIR /S /Q hifi\build-android
MKDIR hifi\build-android
CD hifi\build-android
SET QT_CMAKE_PREFIX_PATH=%QT_CMAKE_PREFIX_PATH_ANDROID%
ECHO QT_CMAKE_PREFIX_PATH=%QT_CMAKE_PREFIX_PATH%
java -version
cmake -G "Unix Makefiles" -DUSE_NSIGHT=0 -DUSE_ANDROID_TOOLCHAIN=1 -DANDROID_QT_CMAKE_PREFIX_PATH=%QT_CMAKE_PREFIX_PATH% ..
IF %ERRORLEVEL% NEQ 0 (
 ECHO Cmake failed
) ELSE (
 ECHO Cmake succeeded
 REM make interface-apk
)
cd .. 
ECHO ON
```

