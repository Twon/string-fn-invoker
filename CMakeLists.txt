cmake_minimum_required(VERSION 3.8)

project(cpp-meetup-2022)

add_subdirectory(lexy)

add_executable(cpp-meetup-2022)

set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/lexy_parse_helpers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/callable_traits.h
)

target_sources(cpp-meetup-2022 PRIVATE ${header_files})
target_sources(cpp-meetup-2022 PRIVATE main.cpp)

target_link_libraries(cpp-meetup-2022 foonathan::lexy::dev)
