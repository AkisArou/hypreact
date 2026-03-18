cmake_minimum_required(VERSION 3.24)
project(hypreact LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(hypreact SHARED
  src/main.cpp
)

target_include_directories(hypreact PRIVATE src)

# Vendored third-party dependencies are pinned as git submodules under third_party/.
# Integration should prefer local source builds over system packages for reproducibility.
#
# Planned integration:
# - QuickJS: build a local static target from quickjs.c and companion sources
# - Yoga: add_subdirectory(third_party/yoga)
# - libcss: wrap the upstream build or provide a local CMake bridge target

set_target_properties(hypreact PROPERTIES
  OUTPUT_NAME hypreact
)
