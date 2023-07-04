cmake_minimum_required(VERSION 3.21)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS ON)

# Set the project name and language
project(
  psim
  VERSION 1.0
  DESCRIPTION "Generalized 2D phonon transport using a Monte Carlo method"
  HOMEPAGE_URL "https://gwgibson.github.io/psim/"
  LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)

include(Dependencies.cmake)
psim_setup_dependencies()

include(ProjectOptions.cmake)

psim_setup_options()
psim_global_options()
psim_local_options()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)


target_compile_features(psim_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(psim::psim_options ALIAS psim_options)
add_library(psim::psim_warnings ALIAS psim_warnings)

target_compile_features(psim_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_subdirectory(./psim)

# Don't even look at tests if we're not top level
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment
# so that it behaves well with MSVC's debugger, and we can run the target from visual studio
if(MSVC)
  get_all_installable_targets(all_targets)
  message("all_targets=${all_targets}")
  set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

# For the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT psim.out)

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(cmake/PackageProject.cmake)

psim_package_project(TARGETS psim.out psim_options psim_warnings)

# Explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(InstallRequiredSystemLibraries)
install(TARGETS psim.out
        RUNTIME DESTINATION bin)
        
set(CPACK_PACKAGE_NAME "psim")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "psim - 2D generalized phonon transport")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "psim")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32 AND NOT UNIX)
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
  set(CPACK_NSIS_MODIFY_PATH ON)
else()
  set(CPACK_GENERATOR "TGZ;DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Graham Gibson gw_gibson@laurentian.ca")
endif()

include(CPack)
