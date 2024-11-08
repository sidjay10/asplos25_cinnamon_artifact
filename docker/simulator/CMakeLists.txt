cmake_minimum_required(VERSION 3.15)

project(sst LANGUAGES C CXX)

include(ExternalProject)

option(SST_ENABLE_PIN "Build SST Elements with Intel PIN tool" OFF)
option(SST_ENABLE_DRAMSIM3 "Build SST Elements with DRAMSIM3" OFF)

#if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(SST_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)
#else()
#  set(SST_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
#endif()

message(STATUS "CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "SST_INSTALL_PREFIX ${SST_INSTALL_PREFIX}")

ExternalProject_Add(sst-core
    PREFIX sst-core
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sst-core"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/sst-core"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/install/sst-core"
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/sst-core && ./autogen.sh
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-core/ && ${CMAKE_CURRENT_SOURCE_DIR}/sst-core/configure --prefix=${SST_INSTALL_PREFIX}/sst-core --disable-mpi
    BUILD_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-core && make all -j
    INSTALL_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-core && make install -j
)

set(SST_DEPS_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deps)

if(SST_ENABLE_PIN)
    ExternalProject_Add(pin
        PREFIX pin
        URL "https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.17-98314-g0c048d619-gcc-linux.tar.gz"
        URL_HASH SHA256=a9cc3df7667b70dd44e51f66ed506bcdc31d942bb7698e1d4e1f8fe5d01c80bb
        INSTALL_DIR "${SST_DEPS_PREFIX}/"
        EXCLUDE_FROM_ALL ON
         CONFIGURE_COMMAND ""
         BUILD_COMMAND ""
         INSTALL_COMMAND mkdir -p ${SST_DEPS_PREFIX}/pin && rm -rf ${SST_DEPS_PREFIX}/pin
         COMMAND cp -r ${CMAKE_CURRENT_BINARY_DIR}/pin/src/pin ${SST_DEPS_PREFIX}
    )

    set(_sst_element_deps ${_sst_element_deps} pin)
    set(_sst_elt_configure_opts ${_sst_elt_configure_opts}--with-pin=${SST_DEPS_PREFIX}/pin)

endif()


if(SST_ENABLE_DRAMSIM3)
    ExternalProject_Add(dramsim3
        PREFIX dramsim3
        GIT_REPOSITORY "https://github.com/umd-memsys/dramsim3.git"
        GIT_TAG "1.0.0"
        SOURCE_DIR "${SST_DEPS_PREFIX}/dramsim3"
        BUILD_IN_SOURCE true
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo
        EXCLUDE_FROM_ALL ON
        INSTALL_COMMAND ""
    )

    set(_sst_element_deps ${_sst_element_deps} dramsim3)
    set(_sst_elt_configure_opts ${_sst_elt_configure_opts} --with-dramsim3=${SST_DEPS_PREFIX}/dramsim3)
endif()

ExternalProject_Add(sst-elements
    PREFIX sst-elements
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sst-elements"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/sst-elements"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/install/sst-elements"
    EXCLUDE_FROM_ALL ON
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/sst-elements && ./autogen.sh
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-elements/ && ${CMAKE_CURRENT_SOURCE_DIR}/sst-elements/configure --prefix=${SST_INSTALL_PREFIX}/sst-elements --with-sst-core=${SST_INSTALL_PREFIX}/sst-core ${_sst_elt_configure_opts}
    BUILD_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-elements && make all -j 64
    INSTALL_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/sst-elements && make install -j 64
    DEPENDS sst-core ${_sst_element_deps}
)



# ExternalProject_Add(custom-elements
#     PREFIX custom-elements
#     SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/custom-elements"
#     BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/custom-elements"
#     INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/install/custom-elements"
#     CMAKE_CACHE_ARGS
#         -DSST_CORE_HOME:PATH=${SST_INSTALL_PREFIX}/sst-core
#         -DCMAKE_INSTALL_PREFIX:PATH=${SST_INSTALL_PREFIX}/custom-elements
#     DEPENDS sst-core
# )

set(SST_CORE_HOME ${SST_INSTALL_PREFIX}/sst-core)
set(CMAKE_INSTALL_PREFIX ${SST_INSTALL_PREFIX}/custom-elements)

add_subdirectory(custom-elements)