# If we're running under the gradle build, HIFI_ANDROID will be set here, but
# ANDROID will not be set until after the `project` statement.  This is the *ONLY*
# place you need to use `HIFI_ANDROID` instead of `ANDROID`
if (WIN32 AND NOT HIFI_ANDROID)
  cmake_minimum_required(VERSION 3.7)
else()
  cmake_minimum_required(VERSION 3.2)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros/TargetPython.cmake")
target_python()

if (HIFI_ANDROID )
    execute_process(
        COMMAND ${HIFI_PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/prebuild.py --android ${HIFI_ANDROID_APP} --build-root ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    execute_process(
        COMMAND ${HIFI_PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/prebuild.py --build-root ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # squelch the Policy CMP0074 warning without requiring an update to cmake 3.12.
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
        cmake_policy(SET CMP0074 NEW)
    endif()
endif()

if(NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg.cmake")
    message(FATAL_ERROR "vcpkg configuration missing.")
endif()

include("${CMAKE_BINARY_DIR}/vcpkg.cmake")
project(hifi)
include("cmake/init.cmake")
include("cmake/compiler.cmake")

add_paths_to_fixup_libs(${VCPKG_INSTALL_ROOT}/bin)
add_paths_to_fixup_libs(${VCPKG_INSTALL_ROOT}/debug/bin)

if (NOT DEFINED CLIENT_ONLY)
  set(CLIENT_ONLY 0)
endif()

if (NOT DEFINED SERVER_ONLY)
  set(SERVER_ONLY 0)
endif()

if (ANDROID OR UWP)
  set(MOBILE 1)
else()
  set(MOBILE 0)
endif()

set(HIFI_USE_OPTIMIZED_IK OFF)
set(BUILD_CLIENT_OPTION ON)
set(BUILD_SERVER_OPTION ON) # CPM WAS ON
set(BUILD_TESTS_OPTION OFF)
set(BUILD_MANUAL_TESTS_OPTION ${BUILD_TESTS_OPTION})
set(BUILD_TOOLS_OPTION ON)
set(BUILD_INSTALLER_OPTION ON)
set(GLES_OPTION OFF)
set(DISABLE_QML_OPTION OFF) # CPM WAS OFF
set(DOWNLOAD_SERVERLESS_CONTENT_OPTION OFF)

if (ANDROID OR UWP)
  set(BUILD_SERVER_OPTION OFF)
  set(BUILD_TOOLS_OPTION OFF)
  set(BUILD_INSTALLER OFF)
endif()

if (CLIENT_ONLY)
  set(BUILD_SERVER_OPTION OFF)
endif()

if (SERVER_ONLY)
  set(BUILD_CLIENT_OPTION OFF)
  set(BUILD_TESTS_OPTION OFF)
endif()

if (ANDROID)
  set(GLES_OPTION ON)
  set(PLATFORM_QT_COMPONENTS AndroidExtras WebView)
  add_definitions(-DHIFI_ANDROID_APP=\"${HIFI_ANDROID_APP}\")
  if (
      (${HIFI_ANDROID_APP} STREQUAL "questInterface") OR 
      (${HIFI_ANDROID_APP} STREQUAL "questFramePlayer") OR
      (${HIFI_ANDROID_APP} STREQUAL "framePlayer") 
    )
    # We know the quest hardware has this extension, so we can force the use of instanced stereo
    add_definitions(-DHAVE_EXT_clip_cull_distance)
    # We can also use multiview stereo techniques
    add_definitions(-DHAVE_OVR_multiview2)
    add_definitions(-DHAVE_OVR_multiview)
    # We can also use our own foveated textures
    add_definitions(-DHAVE_QCOM_texture_foveated)

    # if set, the application itself or some library it depends on MUST implement 
    # `DisplayPluginList getDisplayPlugins()` and `InputPluginList getInputPlugins()`
    add_definitions(-DCUSTOM_INPUT_PLUGINS) 
    add_definitions(-DCUSTOM_DISPLAY_PLUGINS) 
    set(PLATFORM_PLUGIN_LIBRARIES oculusMobile oculusMobilePlugin)
  endif()

  # Allow client code to use preprocessor macros to distinguish between quest and non-quest builds
  if (${HIFI_ANDROID_APP} STREQUAL "questInterface")
    add_definitions(-DANDROID_APP_QUEST_INTERFACE) 
  elseif(${HIFI_ANDROID_APP} STREQUAL "interface")
    add_definitions(-DANDROID_APP_INTERFACE) 
  endif()
else ()
  set(PLATFORM_QT_COMPONENTS WebEngine Xml)
endif ()

if (USE_GLES AND (NOT ANDROID))
  set(DISABLE_QML_OPTION ON)
endif()

option(HIFI_USE_OPTIMIZED_IK "USE OPTIMIZED IK" ${HIFI_USE_OPTIMIZED_IK_OPTION})
option(BUILD_CLIENT "Build client components" ${BUILD_CLIENT_OPTION})
option(BUILD_SERVER "Build server components" ${BUILD_SERVER_OPTION})
option(BUILD_TESTS "Build tests" ${BUILD_TESTS_OPTION})
option(BUILD_MANUAL_TESTS "Build manual tests" ${BUILD_MANUAL_TESTS_OPTION})
option(BUILD_TOOLS "Build tools" ${BUILD_TOOLS_OPTION})
option(BUILD_INSTALLER "Build installer" ${BUILD_INSTALLER_OPTION})
option(USE_GLES "Use OpenGL ES" ${GLES_OPTION})
option(DISABLE_QML "Disable QML" ${DISABLE_QML_OPTION})
option(DISABLE_KTX_CACHE "Disable KTX Cache" OFF)
option(
  DOWNLOAD_SERVERLESS_CONTENT
  "Download and setup default serverless content beside Interface"
  ${DOWNLOAD_SERVERLESS_CONTENT_OPTION}
)

set(PLATFORM_QT_GL OpenGL)

if (USE_GLES)
  add_definitions(-DUSE_GLES)
  add_definitions(-DGPU_POINTER_STORAGE_SHARED)
  set(PLATFORM_GL_BACKEND gpu-gl-common gpu-gles)
else()
  add_definitions(-DGPU_POINTER_STORAGE_RAW)
  set(PLATFORM_GL_BACKEND gpu-gl-common gpu-gl)
endif()

foreach(PLATFORM_QT_COMPONENT ${PLATFORM_QT_COMPONENTS})
  list(APPEND PLATFORM_QT_LIBRARIES "Qt5::${PLATFORM_QT_COMPONENT}")
endforeach()

MESSAGE(STATUS "USE OPTIMIZED IK:      " ${HIFI_USE_OPTIMIZED_IK})
MESSAGE(STATUS "Build server:          " ${BUILD_SERVER})
MESSAGE(STATUS "Build client:          " ${BUILD_CLIENT})
MESSAGE(STATUS "Build tests:           " ${BUILD_TESTS})
MESSAGE(STATUS "Build tools:           " ${BUILD_TOOLS})
MESSAGE(STATUS "Build installer:       " ${BUILD_INSTALLER})
MESSAGE(STATUS "GL ES:                 " ${USE_GLES})
MESSAGE(STATUS "DL serverless content: " ${DOWNLOAD_SERVERLESS_CONTENT})

if (DISABLE_QML)
  MESSAGE(STATUS "QML disabled!")
  add_definitions(-DDISABLE_QML)
endif()

if (DISABLE_KTX_CACHE)
  MESSAGE(STATUS "KTX cache disabled!")
  add_definitions(-DDISABLE_KTX_CACHE)
endif()

if (UNIX AND DEFINED ENV{HIFI_MEMORY_DEBUGGING})
  MESSAGE(STATUS "Memory debugging is enabled")
endif()

#
# Helper projects
#
file(GLOB_RECURSE CMAKE_SRC cmake/*.cmake cmake/CMakeLists.txt)
add_custom_target(cmake SOURCES ${CMAKE_SRC})
GroupSources("cmake")
unset(CMAKE_SRC)

file(GLOB_RECURSE JS_SRC scripts/*.js unpublishedScripts/*.js)
add_custom_target(js SOURCES ${JS_SRC})
GroupSources("scripts")
GroupSources("unpublishedScripts")
unset(JS_SRC)

# Locate the required Qt build on the filesystem
setup_qt()
list(APPEND CMAKE_PREFIX_PATH "${QT_CMAKE_PREFIX_PATH}")

find_package( Threads )

add_definitions(-DGLM_FORCE_RADIANS)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_CTOR_INIT)
if (HIFI_USE_OPTIMIZED_IK)
  MESSAGE(STATUS "SET THE USE IK DEFINITION ")
  add_definitions(-DHIFI_USE_OPTIMIZED_IK)
endif()
set(HIFI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

set(EXTERNAL_PROJECT_PREFIX "project")
set_property(DIRECTORY PROPERTY EP_PREFIX ${EXTERNAL_PROJECT_PREFIX})
setup_externals_binary_dir()

option(USE_NSIGHT "Attempt to find the nSight libraries" 1)

set_packaging_parameters()

# FIXME hack to work on the proper Android toolchain
if (ANDROID)
  add_subdirectory(android/apps/${HIFI_ANDROID_APP})
  return()
endif()

# add subdirectories for all targets
if (BUILD_SERVER)
  add_subdirectory(assignment-client)
  set_target_properties(assignment-client PROPERTIES FOLDER "Apps")
  add_subdirectory(domain-server)
  set_target_properties(domain-server PROPERTIES FOLDER "Apps")
  add_subdirectory(ice-server)
  set_target_properties(ice-server PROPERTIES FOLDER "Apps")
endif()

if (BUILD_CLIENT)
  add_subdirectory(interface)
  set_target_properties(interface PROPERTIES FOLDER "Apps")

  option(USE_SIXENSE "Build Interface with sixense library/plugin" OFF)
endif()

if (BUILD_CLIENT OR BUILD_SERVER)
  add_subdirectory(plugins)
  add_subdirectory(server-console)
endif()

# BUILD_TOOLS option will be handled inside the tools's CMakeLists.txt because 'scribe' tool is required for build anyway
add_subdirectory(tools)

if (BUILD_TESTS)
  # Turn on testing so that add_test works
  # MUST be in the root cmake file for ctest to work 
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
  if (BUILD_MANUAL_TESTS) 
    add_subdirectory(tests-manual)
  endif()
endif()

if (BUILD_INSTALLER)
  if (UNIX)
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/scripts"
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/interface
        COMPONENT ${CLIENT_COMPONENT}
    )
  endif()
  generate_installers()
endif()
