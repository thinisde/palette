cmake_minimum_required(VERSION 3.22)

project(palette VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

find_package(DPP REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${DPP_INCLUDE_DIR}
    /opt/homebrew/include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    /opt/homebrew/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DPP_LIBRARIES}
    dpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-literal-operator)
