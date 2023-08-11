file(GLOB_RECURSE PSIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" # Header files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" # Source files
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" # Header files
        )

set(module_name "psim")
add_library(${module_name} STATIC ${PSIM_SOURCES}) 

find_package(TBB QUIET)

if(TBB_FOUND)
  target_link_libraries(${module_name} PRIVATE TBB::tbb)
else()
  message(WARNING "TBB not found. Building without TBB support.")
endif()

target_link_libraries(
  ${module_name}
  PUBLIC ${module_name}_options ${module_name}_warnings
)

target_include_directories(${module_name} PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_include_directories(${module_name} INTERFACE "${CMAKE_BINARY_DIR}/configured_files/include")

# Precompiled Headers
#target_precompile_headers(${module_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/psimPch.h")

add_executable(${module_name}.out "./src/main.cpp")
target_link_libraries(
  ${module_name}.out
  PUBLIC ${module_name}
  PRIVATE ${module_name}_options ${module_name}_warnings
)
