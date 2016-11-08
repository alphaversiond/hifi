set(TARGET_NAME entities-renderer)
AUTOSCRIBE_SHADER_LIB(gpu model procedural render render-utils)
setup_hifi_library(Widgets Network Script)
link_hifi_libraries(shared gpu procedural model model-networking script-engine render render-utils)

target_bullet()

add_dependency_external_projects(polyvox leopoly)
find_package(PolyVox REQUIRED)
find_package(LeoPoly REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${POLYVOX_INCLUDE_DIRS} ${LEOPOLY_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${POLYVOX_LIBRARIES} ${LEOPOLY_LIBRARIES})
