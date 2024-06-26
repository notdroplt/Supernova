cmake_minimum_required(VERSION 3.12)
project(supernova VERSION 0.1.0 DESCRIPTION "Zenith runtime virtual machine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# generate interface values
add_library(supernova-iface INTERFACE)

set(SNVERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SNVERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SNVERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SNVERSION "${PROJECT_VERSION}")

target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_MAJOR=${SNVERSION_MAJOR}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_MINOR=${SNVERSION_MINOR}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_PATCH=${SNVERSION_PATCH}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION="${SNVERSION}")

target_compile_features(supernova-iface INTERFACE cxx_std_23)
target_compile_features(supernova-iface INTERFACE cxx_constexpr)

include(CTest)

find_package(Doxygen COMPONENTS dot)

if (MSVC)
    add_compile_options(/W4 /Wall /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wformat=2 -pedantic -pedantic-errors)
endif()

add_library(supernova supernova.cxx read_file.cxx)

add_executable(snvm runner.cxx)

target_link_libraries(snvm PUBLIC supernova)

set(DOXYGEN_SEARCHENGINE NO)
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)


if (DOXYGEN_FOUND)
    doxygen_add_docs(doxygen ${PROJECT_SOURCE_DIR} ../docs)
endif()
    

target_link_libraries(supernova INTERFACE supernova-iface)
enable_testing()
add_subdirectory(tests/) 
include(GNUInstallDirs)

install(TARGETS supernova ARCHIVE DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zenith)
install(TARGETS snvm RUNTIME)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_NAME "supernova")
set(CPACK_PACKAGE_VENDOR "notdroplt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Supernova - Zenith runtime virtual machine")
include(CPack)
