cmake_minimum_required(VERSION 3.20)
project(idis_app C)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(BOARD ${ROOT}/infra/boards/stategrid_single_phase)
set(SOC ${ROOT}/infra/soc/v85xxp/v8500)

set(IDIS_PHASES 1 CACHE STRING "Meter phase count for this product")
set(APP_SOURCES main.c startup.c baremetal.c legacy_stubs.c board_ports.c)
add_subdirectory(${BOARD} ${CMAKE_BINARY_DIR}/board)
file(GLOB SOC_SOURCES CONFIGURE_DEPENDS
    ${SOC}/v8500_for_single.c
    ${SOC}/lib_CodeRAM.c
    ${SOC}/lib_LoadNVR.c
    ${SOC}/lib_cortex.c
    ${SOC}/nd_v8500_irq.c
    ${SOC}/vendor/Lib_Driver/src/*.c)
file(GLOB IDIS_SOURCES CONFIGURE_DEPENDS
    ${ROOT}/targets/idis_ip_meter/cosem/*.c
    ${ROOT}/targets/idis_ip_meter/cosem/classes/*.c
    ${ROOT}/targets/idis_ip_meter/mapper_idis/*.c)
file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS ${ROOT}/domain/*.c)
list(FILTER DOMAIN_SOURCES EXCLUDE REGEX "/legacy/")
set(PROTOCOL_SOURCES ${ROOT}/components/protocol/dlms/src/nd_axdr.c)
set(INFRA_SOURCES ${ROOT}/infra/memory/byte_store.c
    ${ROOT}/infra/memory/sector_store.c
    ${ROOT}/infra/kernel/utils.c
    ${ROOT}/infra/kernel/fifo.c
    ${ROOT}/infra/kernel/time_link.c)

add_executable(idis
    ${APP_SOURCES}
    ${SOC_SOURCES}
    ${IDIS_SOURCES}
    ${DOMAIN_SOURCES}
    ${PROTOCOL_SOURCES}
    ${INFRA_SOURCES})

target_include_directories(idis PRIVATE
    ${ROOT}
    ${ROOT}/nidavellir/include
    ${ROOT}/nidavellir/include/kernel
    ${ROOT}/domain
    ${ROOT}/components/protocol/dlms/include
    ${ROOT}/infra
    ${ROOT}/infra/memory
    ${BOARD}
    ${SOC}
    ${SOC}/vendor
    ${SOC}/vendor/Lib_Driver/inc
    ${ROOT}/infra/third_party/arm/cmsis)

target_compile_definitions(idis PRIVATE IDIS_PHASES=${IDIS_PHASES})
target_compile_options(idis PRIVATE
    -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g
    -ffunction-sections -fdata-sections -w -fpermissive)
target_link_libraries(idis PRIVATE board)
target_link_options(idis PRIVATE
    -mcpu=cortex-m0 -mthumb -nostartfiles
    -T ${CMAKE_CURRENT_SOURCE_DIR}/idis.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/idis.map)
set_target_properties(idis PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET idis POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:idis>
            ${CMAKE_BINARY_DIR}/idis.bin
    COMMAND python3 ${ROOT}/tools/size_report.py ${CMAKE_NM}
            $<TARGET_FILE:idis> ${CMAKE_BINARY_DIR}/idis.bin
    VERBATIM)
