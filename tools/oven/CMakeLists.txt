set(TARGET_NAME oven)

setup_hifi_project(Widgets Gui Concurrent)

link_hifi_libraries(shared shaders image gpu ktx fbx hfm baking graphics networking material-networking model-baker task)

setup_memory_debugger()

if (WIN32)
  package_libraries_for_deployment()
elseif (UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(oven PUBLIC "-pthread")
  endif()
elseif (APPLE)
  # Fix up the rpath so macdeployqt works
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
endif()

if (BUILD_SERVER)
  install_beside_console()
endif ()
