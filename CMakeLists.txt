cmake_minimum_required(VERSION 3.12)

find_package(Doxygen COMPONENTS dot)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -Wformat=2 -pedantic -pedantic-errors)

project(supernova VERSION 0.1.0 DESCRIPTION "Virtual machine for the zenith runtime")

add_library(supernova supernova.cxx)

target_compile_features(supernova INTERFACE cxx_std_23)

set(CMAKE_CXX_CLANG_TIDY
  clang-tidy;
  -header-filter=.;
  -checks=*;
  -warnings-as-errors=*;)


set(DOXYGEN_SEARCHENGINE NO)
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)

add_compile_definitions(SUPERNOVA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(SUPERNOVA_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(SUPERNOVA_VERSION_PATCH=${PROJECT_VERSION_PATCH})

if (DOXYGEN_FOUND)
doxygen_add_docs(
    doxygen
    ${PROJECT_SOURCE_DIR}
    ../docs
)
endif()


target_compile_features(supernova PUBLIC cxx_constexpr)