cmake_minimum_required(VERSION 3.14)

project(cpp-meetup-2022)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(LEXY_FORCE_CPP17 ON)

include(FetchContent)
FetchContent_Declare(lexy GIT_REPOSITORY https://github.com/foonathan/lexy.git GIT_TAG 5b7095dab03041b4daf7c6c521118645491b7062)
FetchContent_MakeAvailable(lexy)

add_executable(cpp-meetup-2022)
target_sources(cpp-meetup-2022 
    PRIVATE
        lexy_parse_helpers.h
        callable_traits.h
        main.cpp
)

target_link_libraries(cpp-meetup-2022 PRIVATE foonathan::lexy::dev)

set_target_properties(cpp-meetup-2022
    PROPERTIES
        CMAKE_CXX_STANDARD 17
        CMAKE_CXX_STANDARD_REQUIRED ON
        CMAKE_CXX_EXTENSIONS OFF
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)