cmake_minimum_required(VERSION 3.14)

project("PrpViewer")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE "Release")
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Paths
SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/external/install/lib/cmake/;${CMAKE_CURRENT_LIST_DIR}/external/install/lib/pkgconfig;${CMAKE_CURRENT_LIST_DIR}/external/install/share/cmake/HSPlasma/")

# Dependencies
find_package(PNG REQUIRED)
find_package(HSPlasma REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(glfw3 REQUIRED)
find_package(string_theory REQUIRED)

# Project
file(GLOB_RECURSE SOURCES src/*.c* RECURSE)
file(GLOB_RECURSE HEADERS src/*.h* RECURSE)

file(GLOB HELPERS_FILES src/helpers/*)
source_group(helpers FILES ${HELPERS_FILES})
file(GLOB INPUT_FILES src/input/*)
source_group(input FILES ${INPUT_FILES})
file(GLOB RESOURCES_FILES src/resources/*)
source_group(resources FILES ${RESOURCES_FILES})
file(GLOB_RECURSE LIBS_FILES src/libs/* )
source_group(libs FILES ${LIBS_FILES})
file(GLOB BASE_FILES src/*)
source_group("" FILES ${BASE_FILES})
include_directories(src/libs)
set(SOURCE_FILES ${SOURCES} ${HEADERS} )

add_executable(PrpViewer ${SOURCE_FILES})

target_include_directories(PrpViewer PUBLIC ${HSPlasma_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/external/install/include ${PNG_PNG_INCLUDE_DIR})

target_link_libraries(PrpViewer HSPlasma glfw ${PNG_LIBRARIES} ${STRING_THEORY_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ) 

install(TARGETS PrpViewer RUNTIME DESTINATION bin)
