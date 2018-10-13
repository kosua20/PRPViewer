cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

project("PrpViewer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Paths
SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


# libHSPlasma
ExternalProject_Add(libhsplasmaext
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ../external/libhsplasma
    INSTALL_DIR ${PROJECT_BINARY_DIR}
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
        -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl
        -DBUILD_SHARED_LIBS=OFF
        -DENABLE_PHYSX=OFF
        -DENABLE_PYTHON=OFF
        -DENABLE_NET=OFF
        -DENABLE_TOOLS=OFF)

include_directories(${PROJECT_BINARY_DIR}/include/HSPlasma)
set(HSPLASMA_LIB ${PROJECT_BINARY_DIR}/lib/libHSPlasma.a)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Dependencies
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(Threads REQUIRED)
find_package(string_theory 1.5 REQUIRED)
add_definitions(-DST_NO_2_0_DEPRECATION)
#find_package(OpenGL REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

if(APPLE)
	find_library(CORE_FOUNDATION CoreFoundation)
	if (NOT CORE_FOUNDATION)
		message(FATAL_ERROR "CoreFoundation not found")
	endif()
endif()

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

add_dependencies(PrpViewer libhsplasmaext)
target_link_libraries(PrpViewer ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${STRING_THEORY_LIBRARIES} ${HSPLASMA_LIB} )
target_link_libraries(PrpViewer glfw ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
if(APPLE)
	target_link_libraries(PrpViewer ${CORE_FOUNDATION})
endif()
install(TARGETS PrpViewer RUNTIME DESTINATION bin)
