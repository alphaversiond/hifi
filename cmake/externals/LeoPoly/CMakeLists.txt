include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME leopoly)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-1.zip
        URL_MD5 e2c6425b0332f22a78171d0b3d9e89dd
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/SculptAppDLL.lib CACHE TYPE INTERNAL)

elseif (APPLE)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-Stub.zip
        URL_MD5 a28c434ba3e090d43298aecb1c658db3
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

elseif (ANDROID)
else () # Linux

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-Stub.zip
        URL_MD5 a28c434ba3e090d43298aecb1c658db3
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

endif()
