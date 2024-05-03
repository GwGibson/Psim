file(GLOB_RECURSE PSIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" # Header files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" # Source files
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" # Header files
        )

set(module_name "psim")
set(library_name "${module_name}_library")  # Use module_name to generate library_name

file(GLOB_RECURSE PSIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" # Header files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" # Source files
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" # Header files
        )

add_library(${library_name} STATIC ${PSIM_SOURCES}) 

find_package(TBB QUIET)

if(TBB_FOUND)
  target_link_libraries(${library_name} PRIVATE TBB::tbb)
else()
  message(WARNING "TBB not found. Building without TBB support.")
endif()

target_link_libraries(
  ${library_name}
  PUBLIC ${module_name}_options ${module_name}_warnings
)

target_include_directories(${library_name} PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_include_directories(${library_name} SYSTEM PUBLIC 
    "${nlohmann_json_INCLUDE_DIR}"
)

target_include_directories(${library_name} INTERFACE "${CMAKE_BINARY_DIR}/configured_files/include")

add_executable(${module_name} "./src/main.cpp")
target_link_libraries(
  ${module_name}
  PUBLIC ${library_name} ${module_name}_options ${module_name}_warnings nlohmann_json::nlohmann_json
)
