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

file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS "${BLADE_ROOT}/domain/*.c")
add_library(blade_domain STATIC ${DOMAIN_SOURCES})
target_include_directories(blade_domain PUBLIC ${BLADE_ROOT})

add_library(blade_infra STATIC
    ${BLADE_ROOT}/infra/memory/arena.c
    ${BLADE_ROOT}/infra/memory/byte_store.c
    ${BLADE_ROOT}/infra/memory/parameter_store.c
)
target_include_directories(blade_infra PUBLIC ${BLADE_ROOT})
target_link_libraries(blade_infra PUBLIC blade_domain)

add_library(blade_target_stategrid STATIC
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_energy_codec.c
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_energy_store.c
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_tariff_parser.c
)
target_link_libraries(blade_target_stategrid PUBLIC blade_domain)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.c")
foreach(src ${TEST_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE unity test_support blade_infra blade_target_stategrid)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
