project(rakro LANGUAGES CXX)

file(GLOB_RECURSE RAKRO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/rakro/*.hpp"
)
file(GLOB_RECURSE RAKRO_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/rakro/*.cpp"
)

add_library(rakro
    ${RAKRO_SOURCES}
    ${RAKRO_HEADERS}
)

# Point the PUBLIC include directory to the parent of the "rakro/" folder
target_include_directories(rakro
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(rakro PUBLIC NOMINMAX)


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(rakro PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wundef
        -Wcast-align
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
        -Wdouble-promotion
        -Wformat=2
        -Wnull-dereference
        -Wno-unused-parameter
        -Wno-nullability-extension
        -Wno-nullability-completeness
        -Werror
    )

    message(STATUS "Using Clang, throwing in all warnings")
endif()

