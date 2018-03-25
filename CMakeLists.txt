# the minimum version of CMake required
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

project("PrpViewer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# we add the sub-directories that we want CMake to scan
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

ExternalProject_Add(libhsplasma
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ../external/libhsplasma
    INSTALL_DIR ${PROJECT_BINARY_DIR}
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/../external/libhsplasma-patch/plJPEG.cpp ${PROJECT_BINARY_DIR}/../external/libhsplasma/core/Util/
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
        -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl)

set(HSPLASMA_LIB ${PROJECT_BINARY_DIR}/lib/libHSPlasma.dylib)
include_directories(${PROJECT_BINARY_DIR}/include/HSPlasma)

ExternalProject_Add(libstring_theory
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ../external/string_theory
    INSTALL_DIR ${PROJECT_BINARY_DIR}
    CMAKE_ARGS 
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>)

set(STRING_THEORY_LIB ${PROJECT_BINARY_DIR}/lib/libstring_theory.dylib)
include_directories(${PROJECT_BINARY_DIR}/include/)






# My project
file(GLOB project_sources src/*.cpp)
add_executable(PrpViewer ${project_sources})
add_dependencies(PrpViewer libhsplasma libstring_theory)
target_link_libraries(PrpViewer ${HSPLASMA_LIB} ${STRING_THEORY_LIB})
install(TARGETS PrpViewer RUNTIME DESTINATION bin)
