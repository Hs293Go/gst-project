cmake_minimum_required(VERSION 2.8.3)
project(gst-project)

## Compile as C++11, supported in ROS Kinetic and newer
# add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages

## System dependencies are found with CMake's conventions
find_package(fmt REQUIRED)
find_package(Boost REQUIRED program_options)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

include_directories(
  ${GLIB_INCLUDE_DIRS}
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
)

set(GSTREAMER_MINIMUM_VERSION 1.0.5)

pkg_check_modules(GST1_TEST gstreamer-1.0)
if ( GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    add_definitions(-DGST_API_VERSION_1=1)
else()
    # fallback to gstreamer-0.10
    unset(GSTREAMER_MINIMUM_VERSION)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-0.10)
endif()

include_directories(
  ${GSTREAMER_INCLUDE_DIRS}
  include
)

link_directories(
  ${GSTREAMER_LIBRARY_DIRS}
)

add_executable(gst_stream_udp src/gst_stream_udp.cpp)

target_link_libraries(gst_stream_udp
  ${GSTREAMER_LIBRARIES}
  ${Boost_LIBRARIES}
  fmt::fmt
)
