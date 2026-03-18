cmake_minimum_required(VERSION 3.24)
project(hypreact LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(hypreact SHARED
  src/main.cpp
)

target_include_directories(hypreact PRIVATE src)

# TODO: add Hyprland, QuickJS, Yoga, and libcss dependencies.

set_target_properties(hypreact PROPERTIES
  OUTPUT_NAME hypreact
)
