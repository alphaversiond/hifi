set(TARGET_NAME daydream)
AUTOSCRIBE_SHADER_LIB(gpu daydream)
setup_hifi_library(OpenGL)
link_hifi_libraries(shared plugins ui-plugins gl gpu-gl ui render-utils display-plugins controllers)

target_opengl()

if (ANDROID)
  find_package (GVR)
  if (GVR_FOUND)
    include_directories(${GVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${GVR_LIBRARIES})

    # Expects the gvr-common library folder to be one level outside $ANDROID_APK_BUILD_DIR
    file(WRITE "${ANDROID_APK_BUILD_DIR}/project.properties" "android.library.reference.1=../gvr-common")

  endif()
endif()

GroupSources("src/daydream")
