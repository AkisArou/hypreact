cmake_minimum_required(VERSION 3.24)
project(hypreact LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(HYPREACT_BUILD_YOGA_TESTS "Build vendored Yoga tests" OFF)

include(GNUInstallDirs)

add_compile_definitions(WLR_USE_UNSTABLE)

set(HYPREACT_WARNING_FLAGS
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-value
  -Wno-missing-field-initializers
  -Wno-narrowing
  -Wno-pointer-arith
)

set(HYPREACT_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_library(hypreact_quickjs STATIC
  third_party/quickjs/quickjs.c
  third_party/quickjs/dtoa.c
  third_party/quickjs/libregexp.c
  third_party/quickjs/libunicode.c
  third_party/quickjs/cutils.c
  third_party/quickjs/quickjs-libc.c
)

set_target_properties(hypreact_quickjs PROPERTIES POSITION_INDEPENDENT_CODE ON)

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

target_include_directories(hypreact_quickjs
  SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs
)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZ_TESTS OFF CACHE BOOL "" FORCE)

if(NOT HYPREACT_BUILD_YOGA_TESTS)
  set(YOGA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/yoga")
  add_subdirectory(third_party/yoga/yoga third_party/yoga-build)
else()
  add_subdirectory(third_party/yoga)
endif()

find_package(PkgConfig QUIET)

if(NOT PkgConfig_FOUND)
  message(FATAL_ERROR "pkg-config is required to locate system libcss dependencies")
endif()

pkg_check_modules(LIBCSS REQUIRED libcss)
pkg_check_modules(LIBPARSERUTILS REQUIRED libparserutils)
pkg_check_modules(LIBWAPCAPLET REQUIRED libwapcaplet)
pkg_check_modules(HYPRLAND REQUIRED hyprland)
pkg_check_modules(PIXMAN REQUIRED pixman-1)

add_library(hypreact_libcss_bridge STATIC
  src/style/libcss_bridge.cpp
)

target_include_directories(hypreact_libcss_bridge
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE
    ${LIBCSS_INCLUDE_DIRS}
    ${LIBPARSERUTILS_INCLUDE_DIRS}
    ${LIBWAPCAPLET_INCLUDE_DIRS}
)

target_compile_definitions(hypreact_libcss_bridge PUBLIC HYPREACT_HAS_LIBCSS=1)
target_compile_options(hypreact_libcss_bridge PRIVATE ${HYPREACT_WARNING_FLAGS})
target_link_directories(hypreact_libcss_bridge PRIVATE
  ${LIBCSS_LIBRARY_DIRS}
  ${LIBPARSERUTILS_LIBRARY_DIRS}
  ${LIBWAPCAPLET_LIBRARY_DIRS}
)
target_link_libraries(hypreact_libcss_bridge PRIVATE
  ${LIBCSS_LIBRARIES}
  ${LIBPARSERUTILS_LIBRARIES}
  ${LIBWAPCAPLET_LIBRARIES}
)

add_library(hypreact SHARED
  src/main.cpp
  src/plugin/plugin.cpp
  src/runtime/quickjs_runtime.cpp
  src/layout/layout_engine.cpp
)

target_include_directories(hypreact PRIVATE src)
target_include_directories(hypreact PRIVATE
  ${HYPRLAND_INCLUDE_DIRS}
  ${PIXMAN_INCLUDE_DIRS}
)

target_compile_definitions(hypreact PRIVATE HYPREACT_USE_REAL_HYPRLAND_HEADERS=1)
target_compile_options(hypreact PRIVATE ${HYPREACT_WARNING_FLAGS})

target_link_directories(hypreact PRIVATE
  ${HYPRLAND_LIBRARY_DIRS}
  ${PIXMAN_LIBRARY_DIRS}
)

target_link_libraries(hypreact PRIVATE
  hypreact_quickjs
  hypreact_libcss_bridge
  ${HYPRLAND_LIBRARIES}
  ${PIXMAN_LIBRARIES}
  yogacore
)

if(UNIX AND NOT APPLE)
  target_link_libraries(hypreact PRIVATE dl m pthread)
endif()

set_target_properties(hypreact PROPERTIES
  OUTPUT_NAME hypreact
)

install(TARGETS hypreact LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
