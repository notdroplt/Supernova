add_library(supernova-iface INTERFACE)

set(SNVERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SNVERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SNVERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SNVERSION "${PROJECT_VERSION}")

target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_MAJOR=${SNVERSION_MAJOR}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_MINOR=${SNVERSION_MINOR}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION_PATCH=${SNVERSION_PATCH}LLU)
target_compile_definitions(supernova-iface INTERFACE SUPERNOVA_VERSION="${SNVERSION}")

target_compile_features(supernova-iface INTERFACE cxx_std_17)
target_compile_features(supernova-iface INTERFACE cxx_constexpr)

include_directories(.)
add_library(supernova supernova.cxx)
 
if (MSVC)
    add_compile_options(/W4 /Wall /WX /sdl)
else()
    add_compile_options(-Wall -Wextra -Werror -Wformat=2 -pedantic -pedantic-errors -ffreestanding)
endif()

target_link_libraries(supernova INTERFACE supernova-iface)
