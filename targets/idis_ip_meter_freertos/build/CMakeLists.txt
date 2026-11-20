cmake_minimum_required(VERSION 3.20)
project(idis_freertos_app C)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(BOARD ${ROOT}/targets/idis_ip_meter_freertos/board)
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
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app)
set(APP_SOURCES ${APP_DIR}/main.c ${APP_DIR}/startup.c ${APP_DIR}/legacy_stubs.c ${APP_DIR}/board_ports.c ${APP_DIR}/freertos.c)
add_subdirectory(${BOARD} ${CMAKE_BINARY_DIR}/board)
# The board owns the completion primitive; this product selects the FreeRTOS
# semaphore variant while keeping the polling transports.
target_compile_definitions(board PRIVATE ND_BOARD_USE_FREERTOS=1)
target_include_directories(board PRIVATE
    ${FREERTOS}/include
    ${FREERTOS}/portable/GCC/ARM_CM0
    ${APP_DIR})
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
set(PRODUCT meter_electricity_single_phase_idis_freertos)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/product_manifest.py
            --cmake ${PRODUCT}
            --output ${CMAKE_CURRENT_BINARY_DIR}/product_sources.cmake
    RESULT_VARIABLE PRODUCT_RESULT)
if(NOT PRODUCT_RESULT EQUAL 0)
    message(FATAL_ERROR "failed to resolve product ${PRODUCT}")
endif()
include(${CMAKE_CURRENT_BINARY_DIR}/product_sources.cmake)
list(TRANSFORM PRODUCT_SOURCES PREPEND "${ROOT}/")
# The domain and device-driver closure comes from product.yaml and the
# component manifests; the target no longer compiles every file itself.
list(FILTER PRODUCT_SOURCES INCLUDE REGEX "/domain/|/infra/drivers/|/infra/storage/")
set(LAYER_SOURCES ${PRODUCT_SOURCES})
set(PROTOCOL_SOURCES ${ROOT}/components/protocol/dlms/src/nd_axdr.c)
set(MAINTENANCE_SOURCES ${ROOT}/components/services/maintenance/nd_maintenance.c)
set(INFRA_SOURCES ${ROOT}/infra/memory/byte_store.c
    ${ROOT}/infra/memory/sector_store.c
    ${ROOT}/components/core/types/utils.c
    ${ROOT}/components/core/fifo/fifo.c
    ${ROOT}/infra/soc/v85xxp/v8500/time_link.c)
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
    ${LAYER_SOURCES}
    ${PROTOCOL_SOURCES}
    ${MAINTENANCE_SOURCES}
    ${INFRA_SOURCES}
    ${FREERTOS_SOURCES})

target_include_directories(idis_freertos PRIVATE
    ${FREERTOS}/include
    ${FREERTOS}/portable/GCC/ARM_CM0
    ${APP_DIR}
    ${ROOT}
    ${ROOT}/components/protocol/dlms/include
    ${SOC}
    ${SOC}/vendor
    ${SOC}/vendor/Lib_Driver/inc
    ${ROOT}/infra/third_party/arm/cmsis)

set(ND_ADC_TEMPERATURE_SUPPORT 0 CACHE STRING
    "0 compiles out the temperature ADC channel; IDIS has no caller")
target_compile_definitions(idis_freertos PRIVATE
    IDIS_PHASES=${IDIS_PHASES}
    ND_ADC_TEMPERATURE_SUPPORT=${ND_ADC_TEMPERATURE_SUPPORT})
# Board transport modes are compile-time and selected per product. Defaults
# keep the polling path that has HIL history.
set(ND_BOARD_UART_MODE 0 CACHE STRING "0 poll, 1 irq")
set(ND_BOARD_SPI_MODE 0 CACHE STRING "0 poll, 2 dma")
set(ND_BOARD_IIC_MODE 0 CACHE STRING "0 poll")
foreach(mode_target board idis_freertos)
    target_compile_definitions(${mode_target} PRIVATE
        ND_BOARD_UART_MODE=${ND_BOARD_UART_MODE}
        ND_BOARD_SPI_MODE=${ND_BOARD_SPI_MODE}
        ND_BOARD_IIC_MODE=${ND_BOARD_IIC_MODE})
endforeach()
target_compile_options(idis_freertos PRIVATE
    -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g
    -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables
    -Werror=incompatible-pointer-types)
# The SoC and its vendor driver library are imported as-is. GCC 14 still
# makes an implicit declaration a hard error under -w, so downgrade it for
# the vendor sources only.
set_source_files_properties(${SOC_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-w;-Wno-error=implicit-function-declaration")
target_link_libraries(idis_freertos PRIVATE board)
target_link_options(idis_freertos PRIVATE
    -mcpu=cortex-m0 -mthumb -nostartfiles
    -T ${CMAKE_CURRENT_SOURCE_DIR}/idis.ld
    -Wl,--gc-sections
    -Wl,--sort-section=alignment
    -Wl,-Map=${CMAKE_BINARY_DIR}/idis_freertos.map)
set_target_properties(idis_freertos PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET idis_freertos POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:idis_freertos>
            ${CMAKE_BINARY_DIR}/idis_freertos.bin
    COMMAND python3 ${ROOT}/tools/size_report.py ${CMAKE_NM}
            $<TARGET_FILE:idis_freertos> ${CMAKE_BINARY_DIR}/idis_freertos.bin
    VERBATIM)
