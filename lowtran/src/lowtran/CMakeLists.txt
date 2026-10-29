cmake_minimum_required(VERSION 3.19...3.26)

project(lowtran LANGUAGES Fortran)

enable_testing()

include(cmake/options.cmake)
include(cmake/compilers.cmake)

add_library(lowtran fortran/lowtran7.f)

install(TARGETS lowtran)
