cmake_minimum_required(VERSION 3.20)
project(stategrid_single_phase C)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SOC ${ROOT}/infra/soc/v85xxp/v8500)

set(BOARD_SOURCES
    eeprom.c esam698_v2.c iic_driver.c ndv_Norflash.c ndv_RN8209.c relay.c
    board_io.c iic_pins.c spi_pins.c uart_pins.c rn8209_pins.c esam_pins.c
    relay_pins.c spi_driver.c uart_driver.c)

add_library(board OBJECT ${BOARD_SOURCES})
target_include_directories(board PRIVATE
    ${ROOT}
    ${ROOT}/nidavellir/include
    ${ROOT}/nidavellir/include/kernel
    ${ROOT}/targets/stategrid_meter
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SOC}
    ${SOC}/vendor
    ${SOC}/vendor/Lib_Driver/inc
    ${ROOT}/infra/third_party/arm/cmsis
    ${ROOT}/infra/kernel
    ${ROOT}/infra/memory
    ${ROOT}/domain
    ${ROOT}/application
    ${ROOT}/application/adapters/dl698)
target_compile_options(board PRIVATE
    -mcpu=cortex-m0 -mthumb -ffreestanding -Os -g -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables
    -w -fpermissive)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/board.elf
    COMMAND ${CMAKE_LINKER} -r -o ${CMAKE_BINARY_DIR}/board.elf
            $<TARGET_OBJECTS:board>
    DEPENDS board
    COMMAND_EXPAND_LISTS
    VERBATIM)
add_custom_target(board_elf ALL DEPENDS ${CMAKE_BINARY_DIR}/board.elf)
add_custom_command(TARGET board_elf POST_BUILD
    COMMAND python3 ${ROOT}/tools/size_report.py ${CMAKE_NM}
            ${CMAKE_BINARY_DIR}/board.elf
    VERBATIM)
