cmake_minimum_required(VERSION 3.0.0)
project(gstreamerTest VERSION 0.1.0 LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules (gst REQUIRED IMPORTED_TARGET gstreamer-base-1.0)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${gst_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::gst
)
