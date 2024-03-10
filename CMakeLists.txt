# Set the name and the supported language of the project
project(GPIO_reader C)

# Set the minimum version of cmake required to build this project
cmake_minimum_required(VERSION 3.10)

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

find_library(PAHO_MQTT_LIBRARY paho-mqtt3cs HINTS "/usr/local/lib")

# Add the executable target
add_executable(main main.c)

# Link the executable with GTK3
target_link_libraries(main PRIVATE ${GTK3_LIBRARIES})
target_link_libraries(main ${PAHO_MQTT_LIBRARIES})

# Setup CMake to use GTK3, tell the compiler where to look for headers
# and to the linker where to look for libraries
target_include_directories(main PRIVATE ${GTK3_INCLUDE_DIRS})
target_link_directories(main PRIVATE ${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
target_compile_options(main PRIVATE ${GTK3_CFLAGS_OTHER})