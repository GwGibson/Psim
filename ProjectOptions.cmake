include(cmake/SystemLink.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

macro(psim_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(psim_setup_options)
  option(psim_ENABLE_HARDENING "Enable hardening" OFF)
  cmake_dependent_option(
    psim_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    OFF
    psim_ENABLE_HARDENING
    OFF)

  psim_supports_sanitizers()

  if(ENABLE_DEVELOPER_MODE)
    option(psim_ENABLE_IPO "Enable IPO/LTO" ON)
    option(psim_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(psim_ENABLE_DOXYGEN "Build documentation with Doxygen" OFF)
    option(psim_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(psim_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(psim_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(psim_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(psim_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(psim_ENABLE_PCH "Enable precompiled headers" OFF)
    option(psim_ENABLE_CACHE "Enable ccache" ON)
  else()
    option(psim_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(psim_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(psim_ENABLE_DOXYGEN "Build documentation with Doxygen" OFF)
    option(psim_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(psim_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(psim_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(psim_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(psim_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(psim_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(psim_ENABLE_PCH "Enable precompiled headers" OFF)
    option(psim_ENABLE_CACHE "Enable ccache" OFF)
  endif()

  mark_as_advanced(FORCE ENABLE_DEVELOPER_MODE)

  get_cmake_property(_variableNames VARIABLES)
  foreach (_variableName ${_variableNames})
      string(REGEX MATCH "^(CATCH|CLI11|CMAKE|CPM|FETCHCONTENT|FMT|SPDLOG)" _isMatched ${_variableName})
      if(_isMatched)
          mark_as_advanced(FORCE ${_variableName})
      endif()
  endforeach()  
endmacro()

macro(psim_global_options)
  if(psim_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    psim_enable_ipo()
  endif()

  psim_supports_sanitizers()

  if(psim_ENABLE_HARDENING AND psim_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR psim_ENABLE_SANITIZER_UNDEFINED
       OR psim_ENABLE_SANITIZER_ADDRESS
       OR psim_ENABLE_SANITIZER_THREAD
       OR psim_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${psim_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${psim_ENABLE_SANITIZER_UNDEFINED}")
    psim_enable_hardening(psim_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(psim_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(psim_warnings INTERFACE)
  add_library(psim_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  psim_set_project_warnings(
    psim_warnings
    ${psim_WARNINGS_AS_ERRORS}
    ""
    ""
    "")

  if(psim_ENABLE_DOXYGEN)
    include(cmake/Doxygen.cmake)
    psim_enable_doxygen("awesome-sidebar")
  endif()

  if(psim_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    configure_linker(psim_options)
  endif()

  include(cmake/Sanitizers.cmake)
  psim_enable_sanitizers(
    psim_options
    ${psim_ENABLE_SANITIZER_ADDRESS}
    ${psim_ENABLE_SANITIZER_LEAK}
    ${psim_ENABLE_SANITIZER_UNDEFINED}
    ${psim_ENABLE_SANITIZER_THREAD}
    ${psim_ENABLE_SANITIZER_MEMORY})

  set_target_properties(psim_options PROPERTIES UNITY_BUILD ${psim_ENABLE_UNITY_BUILD})

  if(psim_ENABLE_PCH)
    target_precompile_headers(
      psim_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(psim_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    psim_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(psim_ENABLE_CLANG_TIDY)
    psim_enable_clang_tidy(psim_options ${psim_WARNINGS_AS_ERRORS})
  endif()

  if(psim_ENABLE_CPPCHECK)
    psim_enable_cppcheck(${psim_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(psim_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(psim_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(psim_ENABLE_HARDENING AND NOT psim_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR psim_ENABLE_SANITIZER_UNDEFINED
       OR psim_ENABLE_SANITIZER_ADDRESS
       OR psim_ENABLE_SANITIZER_THREAD
       OR psim_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    psim_enable_hardening(psim_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()