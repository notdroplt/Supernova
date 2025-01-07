cmake_minimum_required(VERSION 3.10)
project(supernova VERSION 0.2.0 DESCRIPTION "Zenith runtime virtual machine")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# generate interface values

add_subdirectory(vm)
include_directories(vm)

include(CTest)

find_package(Doxygen COMPONENTS dot)

if (MSVC)
    add_compile_options(/W4 /Wall /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wformat=2 -pedantic -pedantic-errors)
endif()

add_executable(snvm runner.cxx args.cxx read_file.cxx)

target_link_libraries(snvm PUBLIC supernova)

set(DOXYGEN_SEARCHENGINE NO)
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)


# Set compiler flags for these files


if (DOXYGEN_FOUND)
    doxygen_add_docs(doxygen ${PROJECT_SOURCE_DIR} ../docs)
endif()
    

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
