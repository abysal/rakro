project(rakro_playground LANGUAGES CXX)

add_executable(rakro_playground main.cpp)

target_link_libraries(rakro_playground PRIVATE rakro)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(rakro_playground PRIVATE
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
        -Werror
    )

    message(STATUS "Using Clang, throwing in all warnings")
endif()
