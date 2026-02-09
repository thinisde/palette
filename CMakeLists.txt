# Minimum CMake version required
cmake_minimum_required(VERSION 3.22)

# Project name, version and description
project(palette VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create an executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find our pre-installed DPP package (using FindDPP.cmake).
find_package(DPP REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${DPP_INCLUDE_DIR}
    /opt/homebrew/include
)

# Match: -L/opt/homebrew/lib
target_link_directories(${PROJECT_NAME} PRIVATE
    /opt/homebrew/lib
)

# Match: -ldpp (and also link DPP variables if FindDPP provides them)
target_link_libraries(${PROJECT_NAME}
    ${DPP_LIBRARIES}
    dpp
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
