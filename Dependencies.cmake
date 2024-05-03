include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(psim_setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project

  #  if(NOT TARGET fmtlib::fmtlib)
  #    cpmaddpackage("gh:fmtlib/fmt#9.1.0")
  #  endif()

  #  if(NOT TARGET spdlog::spdlog)
  #    cpmaddpackage(
  #      NAME
  #      spdlog
  #      VERSION
  #      1.11.0
  #      GITHUB_REPOSITORY
  #      "gabime/spdlog"
  #      OPTIONS
  #      "SPDLOG_FMT_EXTERNAL ON")
  #  endif()

  #  if(NOT TARGET Catch2::Catch2WithMain)
  #    cpmaddpackage("gh:catchorg/Catch2@3.3.2")
  #  endif()

  #  if(NOT TARGET CLI11::CLI11)
  #    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  #  endif()
  
   if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json QUIET)

    if(NOT nlohmann_json_FOUND)
      message(STATUS "nlohmann_json not found: Using CPM to fetch nlohmann_json")
      CPMAddPackage(
        NAME nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.4
      )
    set(nlohmann_json_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/single_include" CACHE INTERNAL "")
    endif()
  endif()

  if(NOT TARGET tbb)
    find_package(TBB QUIET)

    if(TBB_FOUND)
      message(STATUS "Found TBB: Using system TBB")
    else()
      message(STATUS "TBB not found: Using CPM to fetch TBB")
      CPMAddPackage(
        NAME tbb
        GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
        GIT_TAG v2021.11.0
        OPTIONS
          "TBB_TEST OFF"
          "TBB_STRICT OFF"
      )
    endif()
  endif()

endfunction()