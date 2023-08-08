set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=skylake-avx512")
endif()

file(GLOB_RECURSE PSIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" # Header files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" # Source files
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" # Header files
        )

set(module_name "psim")
add_library(${module_name} STATIC ${PSIM_SOURCES}) 
target_link_libraries(
  ${module_name}
  PUBLIC psim_options psim_warnings
)

target_include_directories(psim PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_include_directories(psim INTERFACE "${CMAKE_BINARY_DIR}/configured_files/include")

# Precompiled Headers
#target_precompile_headers(${module_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/psimPch.h")

add_executable(psim.out "./src/main.cpp")
target_link_libraries(
  psim.out
  PUBLIC psim
  PRIVATE psim_options psim_warnings
)
