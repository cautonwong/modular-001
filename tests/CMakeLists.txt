cmake_minimum_required(VERSION 3.20)
project(blade_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

set(BLADE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(unity STATIC third_party/unity/src/unity.c)
target_include_directories(unity PUBLIC third_party/unity/src)

add_library(test_support STATIC support/fake_allocator.c)
target_include_directories(test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BLADE_ROOT}
)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.c")
foreach(src ${TEST_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE unity test_support)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
