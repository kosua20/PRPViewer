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


# Dependencies
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(Threads REQUIRED)
find_package(string_theory 1.5 REQUIRED)
add_definitions(-DST_NO_2_0_DEPRECATION)


# Project
file(GLOB project_sources src/*.cpp)
add_executable(PrpViewer ${project_sources})

add_dependencies(PrpViewer libhsplasmaext)
target_link_libraries(PrpViewer ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${STRING_THEORY_LIBRARIES} ${HSPLASMA_LIB} )

install(TARGETS PrpViewer RUNTIME DESTINATION bin)
