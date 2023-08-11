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
  
  #if(NOT TARGET TBB::tbb)
  #  cpmaddpackage(
  #    NAME TBB
  #    VERSION 2021.10.0
  #    URL https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.10.0.tar.gz
  #    OPTIONS
  #      "TBB_TEST OFF")
  #endif()

endfunction()