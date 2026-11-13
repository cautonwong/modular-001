cmake_minimum_required(VERSION 3.20)
project(blade_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

set(BLADE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_custom_target(products_check ALL
    COMMAND ${Python3_EXECUTABLE} ${BLADE_ROOT}/tools/product_manifest.py --check-all
    COMMENT "Checking product component closures"
)

add_library(unity STATIC third_party/unity/src/unity.c)
target_include_directories(unity PUBLIC third_party/unity/src)

add_library(test_support STATIC support/fake_allocator.c support/file_store.c)
target_include_directories(test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BLADE_ROOT}
    ${BLADE_ROOT}/targets/stategrid_meter/app
)

file(GLOB DOMAIN_SOURCES CONFIGURE_DEPENDS
    "${BLADE_ROOT}/domain/common/*.c"
    "${BLADE_ROOT}/domain/metering/*.c"
    "${BLADE_ROOT}/domain/tariff/*.c"
    "${BLADE_ROOT}/domain/load_control/*.c"
    "${BLADE_ROOT}/domain/mbus/*.c"
    "${BLADE_ROOT}/domain/push/*.c"
    "${BLADE_ROOT}/domain/events/*.c"
    "${BLADE_ROOT}/domain/freeze/*.c"
    "${BLADE_ROOT}/domain/display/*.c"
    "${BLADE_ROOT}/domain/report/*.c"
    "${BLADE_ROOT}/domain/security/*.c"
)
add_library(blade_domain STATIC ${DOMAIN_SOURCES})
target_include_directories(blade_domain PUBLIC ${BLADE_ROOT})

add_library(blade_components STATIC
    ${BLADE_ROOT}/components/core/bus/nd_bus.c
    ${BLADE_ROOT}/components/core/buf/nd_buf.c
    ${BLADE_ROOT}/components/services/channel/nd_channel.c
)
target_include_directories(blade_components PUBLIC ${BLADE_ROOT})

add_library(blade_idis_skeleton STATIC
    ${BLADE_ROOT}/components/protocol/dlms/src/nd_axdr.c
    ${BLADE_ROOT}/components/protocol/dlms/src/nd_dlms_server.c
    ${BLADE_ROOT}/components/protocol/dlms/src/nd_dlms_apdu.c
    ${BLADE_ROOT}/components/protocol/dlms/src/nd_dlms_assoc.c
    ${BLADE_ROOT}/components/protocol/dlms/src/nd_dlms_security.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_cosem_port.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_clock.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_register.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_profile_generic.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_data.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_demand.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_disconnect.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_calendar.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_extended_register.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_script_table.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_register_activation.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_function_control.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_push.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_mbus_client.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_mbus_value.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_mbus_port_setup.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_single_action.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_register_monitor.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_limiter.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_image_transfer.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_setup.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/classes/nd_ic_association.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_mbus.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_average.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_pq.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_profile.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_capture.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_energy_store.c
    ${BLADE_ROOT}/targets/idis_ip_meter/mapper_idis/nd_idis_energy.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_profiles.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_profile_table.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_scheduler.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_event.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_profile_store.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_alarm_state.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_event_map.c
    ${BLADE_ROOT}/targets/idis_ip_meter/cosem/nd_idis_event_desc.c
)
target_include_directories(blade_idis_skeleton PUBLIC ${BLADE_ROOT})

add_library(blade_infra STATIC
    ${BLADE_ROOT}/infra/kernel/bitarithm.c
    ${BLADE_ROOT}/infra/kernel/fifo.c
    ${BLADE_ROOT}/infra/kernel/list.c
    ${BLADE_ROOT}/infra/channel/fifo_channel.c
    ${BLADE_ROOT}/infra/memory/arena.c
    ${BLADE_ROOT}/infra/memory/byte_store.c
    ${BLADE_ROOT}/infra/memory/sector_store.c
    ${BLADE_ROOT}/infra/memory/file_store.c
    ${BLADE_ROOT}/infra/memory/parameter_store.c
)
target_include_directories(blade_infra PUBLIC ${BLADE_ROOT})
target_link_libraries(blade_infra PUBLIC blade_domain)

add_library(blade_target_stategrid STATIC
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_energy_codec.c
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_energy_store.c
    ${BLADE_ROOT}/targets/stategrid_meter/persistence/legacy_tariff_parser.c
    ${BLADE_ROOT}/targets/stategrid_meter/app/tariff_lookup.c
)
target_link_libraries(blade_target_stategrid PUBLIC blade_domain)

add_library(blade_application STATIC
    ${BLADE_ROOT}/application/metering/application.c
    ${BLADE_ROOT}/application/metering/demand_registry.c
    ${BLADE_ROOT}/application/adapters/dl698/dl698_time.c
    ${BLADE_ROOT}/application/adapters/dl698/parameter_port.c
    ${BLADE_ROOT}/application/adapters/dl698/report_list.c
    ${BLADE_ROOT}/application/adapters/dl698/report_encoder.c
    ${BLADE_ROOT}/application/adapters/dl698/security_encoder.c
    ${BLADE_ROOT}/application/adapters/dl645/fr_645_matcher.c
    ${BLADE_ROOT}/application/adapters/dl645/fr_645_pack.c
    ${BLADE_ROOT}/application/adapters/dl645/fr_645_data.c
    ${BLADE_ROOT}/application/adapters/dl645/fr_645_addr.c
    ${BLADE_ROOT}/application/adapters/dl645/fr_645_password.c
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_matcher.c
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_pack.c
    ${BLADE_ROOT}/application/adapters/dl698/fr_axdr_codec.c
    ${BLADE_ROOT}/application/ports/platform.c
)
target_include_directories(blade_application PUBLIC ${BLADE_ROOT})
target_link_libraries(blade_application PUBLIC blade_domain)

add_library(blade_target_host STATIC
    ${BLADE_ROOT}/targets/host/composition.c
)
target_link_libraries(blade_target_host PUBLIC blade_application blade_infra blade_target_stategrid blade_components)

add_executable(host_runner ${BLADE_ROOT}/targets/host/host_main.c)
target_link_libraries(host_runner PRIVATE blade_target_host)

add_executable(idis_host_runner ${BLADE_ROOT}/targets/host/idis_host_main.c)
target_link_libraries(idis_host_runner PRIVATE blade_idis_skeleton blade_infra)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.c"
    "${BLADE_ROOT}/targets/idis_ip_meter/tests/unit/*.c"
    "${BLADE_ROOT}/targets/stategrid_meter/tests/unit/*.c")
foreach(src ${TEST_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE unity test_support blade_target_host)
    if(src MATCHES "/targets/idis_ip_meter/tests/unit/"
       OR name STREQUAL "test_dlms_assoc"
       OR name STREQUAL "test_dlms_apdu"
       OR name STREQUAL "test_axdr")
        target_link_libraries(${name} PRIVATE blade_idis_skeleton)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endforeach()

target_include_directories(test_file_persistence PRIVATE
    ${BLADE_ROOT}/nidavellir/include
    ${BLADE_ROOT}/targets/stategrid_meter
)

target_sources(test_fr_698_frame PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_frame.c
)
target_sources(test_fr_698_segment PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_segment.c
)
target_sources(test_fr_698_connect_len PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_connect.c
)
target_sources(test_fr_698_get_len PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_get.c
)
target_sources(test_fr_698_set_len PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_set.c
)
target_sources(test_fr_698_client PRIVATE
    ${BLADE_ROOT}/application/adapters/dl698/fr_698_client.c
)

# ---------------------------------------------------------------------------
# Whole-firmware host simulation
#
# Builds the stategrid_meter application, domain, mapper and resource-manager
# sources from the authoritative IAR project, replacing the board/SOC layers
# with tests/sim stubs and file-backed EEPROM/flash devices. The entry point
# still runs the real baremetal loop; its infinite loop is bounded by
# nd_sim_should_stop().
# ---------------------------------------------------------------------------

option(BLADE_FIRMWARE_SIM_PROFILE "Build firmware_sim with gprof instrumentation" OFF)

set(FIRMWARE_SIM_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/firmware_sim_manifest.cmake")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${BLADE_ROOT}/tools/firmware_sim_manifest.py --cmake
    OUTPUT_FILE ${FIRMWARE_SIM_MANIFEST}
    RESULT_VARIABLE FIRMWARE_SIM_MANIFEST_RESULT
)
if(NOT FIRMWARE_SIM_MANIFEST_RESULT EQUAL 0)
    message(FATAL_ERROR "cannot generate firmware simulation manifest")
endif()
include(${FIRMWARE_SIM_MANIFEST})

add_executable(firmware_sim
    ${FIRMWARE_SIM_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/sim/sim_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sim/sim_trace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sim/sim_devices.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sim/sim_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sim/sim_script.c
    ${CMAKE_CURRENT_SOURCE_DIR}/support/file_store.c
)
target_include_directories(firmware_sim PRIVATE ${FIRMWARE_SIM_INCLUDES})
target_compile_definitions(firmware_sim PRIVATE ND_FIRMWARE_SIM)
target_compile_options(firmware_sim PRIVATE
    -fcommon
    -fno-stack-protector
    -w
    -include "${BLADE_ROOT}/tests/sim/include/sim_compat.h"
)
target_link_options(firmware_sim PRIVATE -no-pie -Wl,--section-start=.sim_periph=0x40000000)
target_link_libraries(firmware_sim PRIVATE m)
set_source_files_properties(
    ${BLADE_ROOT}/targets/stategrid_meter/app/stategrid_meter_entry.c
    PROPERTIES COMPILE_DEFINITIONS "main=nd_firmware_main"
)
if(BLADE_FIRMWARE_SIM_PROFILE)
    target_compile_options(firmware_sim PRIVATE -pg)
    target_link_options(firmware_sim PRIVATE -pg)
endif()

add_test(NAME firmware_sim
    COMMAND firmware_sim --cycles 3
            --eeprom ${CMAKE_CURRENT_BINARY_DIR}/backupfiles/test_eeprom.bin
            --flash ${CMAKE_CURRENT_BINARY_DIR}/backupfiles/test_flash.bin
)

add_test(NAME firmware_acceptance
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/acceptance/run.py
            --cases ${BLADE_ROOT}/targets/stategrid_meter/tests/acceptance
            --sim $<TARGET_FILE:firmware_sim>
            --workdir ${CMAKE_CURRENT_BINARY_DIR}/acceptance/work
            --evidence-dir ${CMAKE_CURRENT_BINARY_DIR}/acceptance/evidence
)
set_tests_properties(firmware_acceptance PROPERTIES
    LABELS acceptance
    TIMEOUT 120
)
