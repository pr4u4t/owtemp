# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include_directories(${CMAKE_SOURCE_DIR}/managed_components/espressif__ds18b20/include)
include_directories(${CMAKE_SOURCE_DIR}/managed_components/espressif__onewire_bus/include)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(owtemp)
