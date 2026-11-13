cmake_minimum_required(VERSION 3.20)
project(idis_freertos_app C)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(BOARD ${ROOT}/infra/boards/stategrid_single_phase)
set(SOC ${ROOT}/infra/soc/v85xxp/v8500)
set(FREERTOS_KERNEL_DIR
    "${ROOT}/infra/third_party/freertos/FreeRTOS-KernelV11.3.3"
    CACHE PATH "FreeRTOS 11.3.3 kernel checkout")
set(FREERTOS ${FREERTOS_KERNEL_DIR})
if(NOT EXISTS "${FREERTOS}/tasks.c")
    message(FATAL_ERROR
        "FreeRTOS kernel not found at ${FREERTOS}. Set FREERTOS_KERNEL_DIR to a FreeRTOS 11.3.3 checkout.")
endif()

set(IDIS_PHASES 1 CACHE STRING "Meter phase count for this product")
set(APP_SOURCES main.c startup.c legacy_stubs.c board_ports.c freertos.c)
add_subdirectory(${BOARD} ${CMAKE_BINARY_DIR}/board)
file(GLOB SOC_SOURCES CONFIGURE_DEPENDS
    ${SOC}/v8500_for_single.c
    ${SOC}/lib_CodeRAM.c
    ${SOC}/lib_LoadNVR.c
    ${SOC}/lib_cortex.c
    ${SOC}/nd_v8500_irq.c
    ${SOC}/vendor/Lib_Driver/src/*.c)
file(GLOB IDIS_SOURCES CONFIGURE_DEPENDS
    ${ROOT}/targets/idis_ip_meter_freertos/cosem/*.c
    ${ROOT}/targets/idis_ip_meter_freertos/cosem/classes/*.c
    ${ROOT}/targets/idis_ip_meter_freertos/mapper_idis/*.c)
file(GLOB_RECURSE DOMAIN_SOURCES CONFIGURE_DEPENDS ${ROOT}/domain/*.c)
list(FILTER DOMAIN_SOURCES EXCLUDE REGEX "/legacy/")
set(PROTOCOL_SOURCES ${ROOT}/components/protocol/dlms/src/nd_axdr.c)
set(INFRA_SOURCES ${ROOT}/infra/memory/byte_store.c
    ${ROOT}/infra/memory/sector_store.c
    ${ROOT}/infra/kernel/utils.c
    ${ROOT}/infra/kernel/fifo.c
    ${ROOT}/infra/kernel/time_link.c)
set(FREERTOS_SOURCES
    ${FREERTOS}/tasks.c
    ${FREERTOS}/queue.c
    ${FREERTOS}/list.c
    ${FREERTOS}/portable/GCC/ARM_CM0/port.c
    ${FREERTOS}/portable/GCC/ARM_CM0/portasm.c
    ${FREERTOS}/portable/GCC/ARM_CM0/mpu_wrappers_v2_asm.c
    ${FREERTOS}/portable/MemMang/heap_1.c)

add_executable(idis_freertos
    ${APP_SOURCES}
    ${SOC_SOURCES}
    ${IDIS_SOURCES}
    ${DOMAIN_SOURCES}
    ${PROTOCOL_SOURCES}
    ${INFRA_SOURCES}
    ${FREERTOS_SOURCES})

target_include_directories(idis_freertos PRIVATE
    ${FREERTOS}/include
    ${FREERTOS}/portable/GCC/ARM_CM0
    ${CMAKE_CURRENT_SOURCE_DIR}
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

target_compile_definitions(idis_freertos PRIVATE IDIS_PHASES=${IDIS_PHASES})
target_compile_options(idis_freertos PRIVATE
    -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g
    -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -w -fpermissive)
target_link_libraries(idis_freertos PRIVATE board)
target_link_options(idis_freertos PRIVATE
    -mcpu=cortex-m0 -mthumb -nostartfiles
    -T ${CMAKE_CURRENT_SOURCE_DIR}/idis.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/idis_freertos.map)
set_target_properties(idis_freertos PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET idis_freertos POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:idis_freertos>
            ${CMAKE_BINARY_DIR}/idis_freertos.bin
    COMMAND python3 ${ROOT}/tools/size_report.py ${CMAKE_NM}
            $<TARGET_FILE:idis_freertos> ${CMAKE_BINARY_DIR}/idis_freertos.bin
    VERBATIM)
