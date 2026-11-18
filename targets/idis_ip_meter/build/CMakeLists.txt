cmake_minimum_required(VERSION 3.20)
project(idis_app C)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(BOARD ${ROOT}/targets/idis_ip_meter/board)
set(SOC ${ROOT}/infra/soc/v85xxp/v8500)

set(IDIS_PHASES 1 CACHE STRING "Meter phase count for this product")
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../app)
set(APP_SOURCES ${APP_DIR}/main.c ${APP_DIR}/startup.c ${APP_DIR}/baremetal.c ${APP_DIR}/legacy_stubs.c ${APP_DIR}/board_ports.c)
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
set(PRODUCT meter_electricity_single_phase_idis)
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
list(FILTER PRODUCT_SOURCES INCLUDE REGEX "/domain/|/infra/drivers/")
set(LAYER_SOURCES ${PRODUCT_SOURCES})
set(PROTOCOL_SOURCES ${ROOT}/components/protocol/dlms/src/nd_axdr.c)
set(INFRA_SOURCES ${ROOT}/infra/memory/byte_store.c
    ${ROOT}/infra/memory/sector_store.c
    ${ROOT}/components/core/types/utils.c
    ${ROOT}/components/core/fifo/fifo.c
    ${ROOT}/infra/soc/v85xxp/v8500/time_link.c)

add_executable(idis
    ${APP_SOURCES}
    ${SOC_SOURCES}
    ${IDIS_SOURCES}
    ${LAYER_SOURCES}
    ${PROTOCOL_SOURCES}
    ${INFRA_SOURCES})

target_include_directories(idis PRIVATE
    ${ROOT}
    ${ROOT}/components/protocol/dlms/include
    ${SOC}
    ${SOC}/vendor
    ${SOC}/vendor/Lib_Driver/inc
    ${ROOT}/infra/third_party/arm/cmsis)

target_compile_definitions(idis PRIVATE IDIS_PHASES=${IDIS_PHASES})
# Board transport modes are compile-time and selected per product. Defaults
# keep the polling path that has HIL history.
set(ND_BOARD_UART_MODE 0 CACHE STRING "0 poll, 1 irq")
set(ND_BOARD_SPI_MODE 0 CACHE STRING "0 poll, 2 dma")
set(ND_BOARD_IIC_MODE 0 CACHE STRING "0 poll")
foreach(mode_target board idis)
    target_compile_definitions(${mode_target} PRIVATE
        ND_BOARD_UART_MODE=${ND_BOARD_UART_MODE}
        ND_BOARD_SPI_MODE=${ND_BOARD_SPI_MODE}
        ND_BOARD_IIC_MODE=${ND_BOARD_IIC_MODE})
endforeach()
target_compile_options(idis PRIVATE
    -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g
    -ffunction-sections -fdata-sections
    -Werror=incompatible-pointer-types)
# The SoC and its vendor driver library are imported as-is. GCC 14 still
# makes an implicit declaration a hard error under -w, so downgrade it for
# the vendor sources only.
set_source_files_properties(${SOC_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-w;-Wno-error=implicit-function-declaration")
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
