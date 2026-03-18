cmake_minimum_required(VERSION 3.24)
project(hypreact LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(HYPREACT_BUILD_YOGA_TESTS "Build vendored Yoga tests" OFF)
option(HYPREACT_REQUIRE_LIBCSS_DEPS "Require libcss support deps at configure time" OFF)

set(HYPREACT_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_library(hypreact_quickjs STATIC
  third_party/quickjs/quickjs.c
  third_party/quickjs/dtoa.c
  third_party/quickjs/libregexp.c
  third_party/quickjs/libunicode.c
  third_party/quickjs/cutils.c
  third_party/quickjs/quickjs-libc.c
)

target_include_directories(hypreact_quickjs
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs
)

target_compile_definitions(hypreact_quickjs
  PRIVATE
    _GNU_SOURCE
    CONFIG_VERSION="hypreact-vendored"
)

target_compile_options(hypreact_quickjs PRIVATE -fwrapv -std=gnu99)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZ_TESTS OFF CACHE BOOL "" FORCE)

if(NOT HYPREACT_BUILD_YOGA_TESTS)
  set(YOGA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/yoga")
  add_subdirectory(third_party/yoga/yoga third_party/yoga-build)
else()
  add_subdirectory(third_party/yoga)
endif()

find_package(PkgConfig QUIET)

set(HYPREACT_LIBCSS_AVAILABLE FALSE)
if(PkgConfig_FOUND)
  pkg_check_modules(LIBPARSERUTILS QUIET libparserutils)
  pkg_check_modules(LIBWAPCAPLET QUIET libwapcaplet)
  if(LIBPARSERUTILS_FOUND AND LIBWAPCAPLET_FOUND)
    set(HYPREACT_LIBCSS_AVAILABLE TRUE)
  endif()
endif()

add_library(hypreact_libcss_bridge STATIC
  src/style/libcss_bridge.cpp
)

target_include_directories(hypreact_libcss_bridge
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libcss/include
)

if(HYPREACT_LIBCSS_AVAILABLE)
  target_compile_definitions(hypreact_libcss_bridge PUBLIC HYPREACT_HAS_LIBCSS=1)
  target_include_directories(hypreact_libcss_bridge PRIVATE
    ${LIBPARSERUTILS_INCLUDE_DIRS}
    ${LIBWAPCAPLET_INCLUDE_DIRS}
  )
  target_link_directories(hypreact_libcss_bridge PRIVATE
    ${LIBPARSERUTILS_LIBRARY_DIRS}
    ${LIBWAPCAPLET_LIBRARY_DIRS}
  )
  target_link_libraries(hypreact_libcss_bridge PRIVATE
    ${LIBPARSERUTILS_LIBRARIES}
    ${LIBWAPCAPLET_LIBRARIES}
  )
elseif(HYPREACT_REQUIRE_LIBCSS_DEPS)
  message(FATAL_ERROR "libcss dependencies not found. Install libparserutils and libwapcaplet or configure with HYPREACT_REQUIRE_LIBCSS_DEPS=OFF.")
else()
  message(STATUS "libcss support dependencies not found; building stub libcss bridge only")
endif()

add_library(hypreact SHARED
  src/main.cpp
)

target_include_directories(hypreact PRIVATE src)

target_link_libraries(hypreact PRIVATE
  hypreact_quickjs
  hypreact_libcss_bridge
  yogacore
)

if(UNIX AND NOT APPLE)
  target_link_libraries(hypreact PRIVATE dl m pthread)
endif()

set_target_properties(hypreact PROPERTIES
  OUTPUT_NAME hypreact
)
