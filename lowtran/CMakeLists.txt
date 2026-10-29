cmake_minimum_required(VERSION 3.19...3.26)

project(lowtran LANGUAGES Fortran)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTING "Build testing tools" ON)

file(GENERATE OUTPUT .gitignore CONTENT "*")

add_subdirectory(src/lowtran)