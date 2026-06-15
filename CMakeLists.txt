cmake_minimum_required(VERSION 3.28)

# Setup C compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Setup C++ compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Set the project name
set(CMAKE_PROJECT_NAME sam-threadx)

# Project settings
project(${CMAKE_PROJECT_NAME})

# Add the prject target
add_library(${CMAKE_PROJECT_NAME} STATIC)

# Add all sub directories
target_sources(${CMAKE_PROJECT_NAME} PRIVATE sam-filex.c)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE sam-threadx.c)

# Add self include path
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link the target reference libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE sam)

