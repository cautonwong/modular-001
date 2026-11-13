#!/usr/bin/env python3
import csv
import re

ROWS = list(csv.DictReader(open('docs/idis/obis_members.csv', encoding='utf-8')))

P = []


def prof(pid, patterns, state, data, deps, read, write, action, existing, embed,
         persist, gap, scaler='', member_map=None):
    P.append(dict(id=pid, patterns=patterns, state=state, data=data, deps=deps,
                  read=read, write=write, action=action, existing=existing,
                  embed=embed, persist=persist, gap=gap, scaler=scaler,
                  member_map=member_map or {}))


prof('CLOCK', [r'^0-0:1\.0\.0\.255$'],
     'target clock adapter + domain/common/calendar',
     'nd_datetime_t (second, minute, hour, day, month, year); target timezone/DST config',
     'domain/common/calendar; target RTC; EEPROM parameters',
     'nd_cosem_port.get -> nd_ic_clock_get -> target clock adapter -> RTC -> nd_datetime_t -> nd_axdr date-time',
     'SET -> nd_ic_clock_set -> nd_calendar_date_valid/time_valid -> target RTC set',
     'none',
     'domain/common/calendar.h:24,26,28,33; targets/stategrid_meter/app/rtos/time_link.c; targets/stategrid_meter/app/driver_entry.c:118',
     'reuse calendar; add target clock port and IDIS clock class; move timezone/DST out of legacy control block',
     'RTC backup + EEPROM timezone/DST',
     'No clock port, no timezone/DST object state, RTC error status not exposed',
     scaler='none')

prof('LOCAL_TIME', [r'^1-0:0\.9\.(1|2)\.255$'],
     'target clock adapter + domain/common/calendar',
     'nd_datetime_t derived from UTC/local clock and timezone offset',
     'domain/common/calendar; target RTC',
     'nd_cosem_port.get -> nd_ic_data_get -> target clock adapter -> local time/date',
     'read-only',
     'none',
     'domain/common/calendar.h:28,33; no local-time adapter',
     'new target local-time view; reuse calendar arithmetic',
     'none',
     'No timezone/DST state; local time/date are not derived from clock object')

prof('TIME_SHIFT_LIMIT', [r'^1-0:0\.9\.11\.255$'],
     'target clock config',
     'int16_t/uint16_t time shift limit in seconds',
     'domain/common/calendar; EEPROM parameter store',
     'nd_cosem_port.get -> nd_ic_register_get -> target clock config',
     'SET -> class handler -> EEPROM parameter store',
     'none',
     'EEPROM parameter store; no IDIS clock config record',
     'new parameter record; reuse nd_parameter_store_t',
     'EEPROM',
     'No IDIS parameter id/record layout')

prof('TARIFF_SCRIPT', [r'^0-0:10\.0\.(100|1)\.255$'],
     'target script engine + domain/tariff',
     'nd_script_table_t / nd_script_t / nd_action_spec_t; script 1..4 actions',
     'domain/tariff; target script engine; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate action specs -> EEPROM/parameter store',
     'ACTION {member} -> nd_script_execute -> port.set/action -> tariff/register objects',
     'No generic script engine; legacy 698 action tables in targets/stategrid_meter/mapper_698/map_698_action',
     'new target script engine and script table store; reuse COSEM port for actions',
     'EEPROM/parameter store',
     'Script table storage and validation absent; MDI reset action list unresolved')

prof('LOAD_SCRIPT', [r'^0-0:10\.0\.(103|106)\.255$', r'^0-1:10\.0\.106\.255$'],
     'target script engine + domain/load_control',
     'nd_script_table_t; class 70 SET/ACTION specs',
     'domain/load_control; target script engine; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate -> parameter store',
     'ACTION {member} -> nd_script_execute -> port.set/action -> disconnect objects',
     'No generic script engine; relay adapter exists at targets/stategrid_meter/app/relay_control_adapter.c:35',
     'new script engine; reuse load_control state and port',
     'EEPROM/parameter store',
     'Remote execute must be denied; script/action list and M-Bus variants need storage')

prof('PUSH_SCRIPT', [r'^0-0:10\.0\.108\.255$'],
     'target script engine + push engine',
     'nd_script_table_t; class 40 push actions',
     'target push engine; nd_cosem_port; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate -> parameter store',
     'ACTION {member} -> nd_script_execute -> nd_ic_push_action',
     'No push engine; no script engine',
     'new push engine and script table',
     'EEPROM/parameter store',
     'No Push object or Data Notification path')

prof('FUNCTION_SCRIPT', [r'^0-0:10\.0\.111\.255$', r'^0-1:10\.0\.111\.255$'],
     'target script engine + function-control state',
     'nd_script_table_t; class 122 set_function_status actions',
     'target function control; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate -> parameter store',
     'ACTION {member} -> nd_script_execute -> port.action(122)',
     'No function-control state or script engine',
     'new function control and script table',
     'EEPROM/parameter store',
     'Function list/activation status semantics not implemented')

prof('IMAGE_SCRIPT', [r'^0-0:10\.0\.107\.255$'],
     'target script engine + image transfer',
     'nd_script_table_t; class 18 image_activate action',
     'target upgrade; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate -> parameter store',
     'ACTION {member} -> nd_script_execute -> port.action(18, image_activate)',
     'Legacy upgrade in targets/stategrid_meter/upgrade/upgrade.c; no script engine',
     'new script engine; adapt upgrade image activation',
     'EEPROM/parameter store',
     'Image activation action list missing from IDIS object model')

prof('MBUS_PROFILE_SCRIPT', [r'^0-b:10\.0\.110\.255$'],
     'target script engine + M-Bus profile control',
     'nd_script_table_t; M-Bus profile capture gate actions',
     'M-Bus engine; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_script_get -> script table state',
     'SET scripts -> validate -> parameter store',
     'ACTION {member} -> nd_script_execute -> M-Bus profile control',
     'No M-Bus engine or script engine',
     'new M-Bus profile control; content unresolved',
     'EEPROM/parameter store',
     'IDIS document does not print the action list')

prof('SPECIAL_DAYS', [r'^0-0:11\.', r'^0-1:11\.'],
     'target tariff/load-control calendar state',
     'nd_tariff_holiday_t holidays[20] today; IDIS needs day_id entries array[0..30]',
     'domain/tariff; EEPROM parameter store',
     'nd_cosem_port.get -> nd_ic_special_days_get -> target calendar state',
     'SET entries -> validate -> EEPROM/parameter store',
     'none',
     'domain/tariff/tariff.h:20-28; legacy_tariff_parser.c',
     'reuse holiday concept; add IDIS special-days record with day_id',
     'EEPROM/parameter store',
     'Max 20 vs 30 entries; IDIS day_id mapping not implemented')

prof('ACTIVITY_CALENDAR', [r'^0-0:13\.', r'^0-1:13\.'],
     'target tariff/load-control calendar state',
     'nd_tariff_table_t has seasons/day tables/periods; IDIS needs full active/passive season/week/day profiles',
     'domain/tariff; EEPROM parameter store',
     'nd_cosem_port.get -> nd_ic_activity_calendar_get -> target calendar state',
     'SET active/passive profiles -> validate -> EEPROM/parameter store',
     'ACTION activate_passive_calendar -> swap active/passive -> EEPROM',
     'domain/tariff/tariff.h:26-55; tariff_lookup.c:16,21; legacy_tariff_parser.c',
     'reuse nd_tariff_table_t for active lookup; add passive/active IDIS calendar model',
     'EEPROM/parameter store',
     'No passive table, week profile, or activate method; 4 calendar objects')

prof('REGISTER_ACTIVATION', [r'^0-0:14\.'],
     'target tariff/register activation state',
     'register_assignment array; mask_list array; active_mask octet_string[0..8]',
     'domain/tariff; domain/metering; EEPROM',
     'nd_cosem_port.get -> nd_ic_register_activation_get -> target activation state',
     'SET assignment/mask -> validate -> EEPROM',
     'ACTION add_mask/delete_mask -> update active_mask',
     'Legacy mbase control block TOUNo; metering_add_pulses tariff slot; no IDIS activation object',
     'new target activation state; reuse tariff slot and metering ledger',
     'EEPROM/parameter store',
     'No assignment/mask state or add/remove mask methods')

prof('SCHEDULER', [r'^0-0:15\.', r'^0-1:15\.0\.1\.255$', r'^0-1:15\.0\.7\.255$'],
     'target scheduler + script engine',
     'nd_single_action_schedule_t: executed_script, type, execution_time[]',
     'target scheduler; domain/freeze; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_single_action_get -> schedule state',
     'SET schedule -> validate -> EEPROM',
     'ACTION {member} -> script table -> nd_script_execute',
     'domain/freeze/schedule.h:34,39,43 for due calculations; no IDIS scheduler state',
     'new scheduler state; reuse freeze due helpers',
     'EEPROM',
     'Scheduler table/trigger mapping not implemented')

prof('PUSH_SCHEDULER', [r'^0-[1-4]:15\.0\.4\.255$'],
     'target scheduler + push engine',
     'nd_single_action_schedule_t pointing at push script/table',
     'target scheduler; push engine; nd_cosem_port',
     'nd_cosem_port.get -> nd_ic_single_action_get -> schedule state',
     'SET schedule -> validate -> EEPROM',
     'ACTION {member} -> push script -> nd_ic_push_action',
     'No scheduler or push engine',
     'new scheduler and push engine',
     'EEPROM',
     'Push scheduler interval semantics missing')

prof('ALARM_MONITOR', [r'^0-0:16\.1\.'],
     'target alarm state + domain/events',
     'thresholds array; monitored_value; actions array; nd_generic_event_state_t/over_current state',
     'domain/events; target alarm engine',
     'nd_cosem_port.get -> nd_ic_register_monitor_get -> target alarm state -> monitored value via port',
     'SET thresholds/actions -> validate -> EEPROM',
     'none',
     'domain/events/generic.h:14; over_current.h:18; no IDIS alarm monitor',
     'new target alarm monitor; reuse event debounce primitives',
     'EEPROM',
     'Threshold/action mapping and alarm register/descriptor integration missing')

prof('LIMITER', [r'^0-0:17\.0\.0\.255$'],
     'target limiter state + domain/load_control',
     'monitored_value; threshold_active/normal/emergency; durations; emergency profile; actions',
     'domain/load_control; target limiter',
     'nd_cosem_port.get -> nd_ic_limiter_get -> target limiter state',
     'SET thresholds/actions -> EEPROM',
     'none',
     'load_control.h:19-28,61,80; no limiter object',
     'new limiter state; reuse load_control relay actions',
     'EEPROM',
     'Limiter thresholds/actions not implemented')

prof('MODEM', [r'^0-0:2\.0\.0\.255$'],
     'target modem config',
     'comm_speed; initialization_string; modem_profile',
     'target GPRS/modem; UART',
     'nd_cosem_port.get -> nd_ic_modem_get -> target modem config',
     'SET config -> validate -> EEPROM',
     'none',
     'No modem driver; UART board only',
     'new target modem config and PPP driver',
     'EEPROM',
     'No modem hardware/driver')

prof('AUTO_CONNECT', [r'^0-0:2\.1\.0\.255$', r'^0-0:2\.2\.1\.255$'],
     'target auto-connect state + nd_ip_port',
     'mode, repetitions, repetition_delay, calling_window, destination_list',
     'target net; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_auto_connect_get -> target net config',
     'SET config -> EEPROM',
     'ACTION connect -> nd_ip_port connect/open',
     'No IP stack or auto-connect state',
     'new target auto-connect; reuse nd_ip_port',
     'EEPROM',
     'No IP stack; modes 101..104 undefined in code')

prof('AUTO_ANSWER', [r'^0-0:2\.2\.0\.255$'],
     'target auto-answer state',
     'mode, listening_window, status, number_of_calls/rings, allowed_callers',
     'target GPRS/modem',
     'nd_cosem_port.get -> nd_ic_auto_answer_get -> target modem state',
     'SET config -> EEPROM',
     'none',
     'No modem driver',
     'new target auto-answer state',
     'EEPROM',
     'No modem hardware/driver')

prof('LOCAL_PORT', [r'^0-0:20\.0\.0\.255$', r'^0-1:20\.0\.0\.255$'],
     'target local port config + nd_dlms_link',
     'default_mode, baud, response_time, device_addr, passwords',
     'target UART; nd_dlms_link',
     'nd_cosem_port.get -> nd_ic_local_port_get -> target UART config',
     'SET config -> EEPROM',
     'none',
     'UART board/uart_driver.c; no IEC local port object',
     'new target local-port config; reuse UART driver',
     'EEPROM',
     'Passwords/P1/P2/P3 and IEC handshake not implemented')

prof('DISPLAY_PROFILE', [r'^0-0:21\.0\.'],
     'target display model + profile buffer',
     'display list/capture objects; profile buffer',
     'domain/display; target LCD',
     'nd_cosem_port.get -> nd_ic_profile_get -> target display profile',
     'SET capture objects/period -> EEPROM',
     'ACTION capture -> refresh display list',
     'domain/display only backlight; no display model',
     'new display model; optional first phase',
     'EEPROM',
     'Display objects optional; no LCD model')

prof('HDLC_SETUP', [r'^0-0:22\.0\.0\.255$', r'^0-1:22\.0\.0\.255$'],
     'target HDLC config + nd_dlms_link',
     'comm_speed, windows, max info, timeouts, device_address',
     'nd_dlms_link; target UART',
     'nd_cosem_port.get -> nd_ic_hdlc_setup_get -> target HDLC config',
     'SET config -> EEPROM',
     'none',
     'No nd_hdlc_link implementation yet; UART board',
     'new HDLC link and config; reuse UART',
     'EEPROM',
     'HDLC link not implemented')

prof('MBUS_PORT', [r'^0-0:24\.6\.', r'^0-1:24\.6\.'],
     'target M-Bus link + nd_mbus_link_t',
     'comm_speed; M-Bus master port state',
     'components/protocol/mbus; target UART',
     'nd_cosem_port.get -> nd_ic_mbus_port_get -> target M-Bus link',
     'SET comm_speed -> EEPROM',
     'none',
     'No M-Bus component/hardware; UART board',
     'new M-Bus link and port object',
     'EEPROM',
     'No M-Bus transceiver/hardware')

prof('TCP_UDP', [r'^0-0:25\.0\.0\.255$'],
     'target IP config + nd_ip_port',
     'TCP-UDP_port, IP_reference, MSS, nb_conn, inactivity_timeout',
     'target net; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_tcp_udp_get -> target net config',
     'SET config -> EEPROM',
     'none',
     'No IP stack; nd_ip_port planned',
     'new target net config; reuse nd_ip_port',
     'EEPROM',
     'No IP stack')

prof('IPV4', [r'^0-0:25\.1\.0\.255$'],
     'target IPv4 config + nd_ip_port',
     'DL_reference, IP_address, multicast, options, subnet, gateway, DHCP, DNS',
     'target net; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_ipv4_get -> target net config',
     'SET config -> EEPROM',
     'ACTION {member} -> target net',
     'No IP stack',
     'new target IPv4 config; reuse nd_ip_port',
     'EEPROM',
     'No IPv4 stack; method mapping undefined')

prof('IPV6', [r'^0-0:25\.7\.0\.255$'],
     'target IPv6 config + nd_ip_port',
     'DL_reference, address_mode, unicast/multicast, gateway, DNS, traffic_class, neighbor',
     'target net; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_ipv6_get -> target net config',
     'SET config -> EEPROM',
     'ACTION {member} -> target net',
     'No IPv6 stack',
     'new target IPv6 config; reuse nd_ip_port',
     'EEPROM',
     'No IPv6/G3 6LoWPAN stack')

prof('NTP', [r'^0-0:25\.10\.'],
     'target NTP client + domain/common/calendar',
     'activated, server_address, port, auth_method, keys, client_key',
     'target/services.time; domain/common/calendar; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_ntp_get -> target NTP config',
     'SET config -> EEPROM',
     'ACTION {member} -> NTP request -> clock set',
     'No NTP service',
     'new optional target NTP client',
     'EEPROM',
     'Optional; no NTP stack')

prof('MAC', [r'^0-0:25\.2\.0\.255$'],
     'target board MAC + nd_ip_port',
     'MAC_address octet_string',
     'target board; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_mac_get -> board MAC',
     'read-only',
     'none',
     'board_io.h has no MAC API; no Ethernet board',
     'new board MAC getter; target net binding',
     'none',
     'No MAC/PHY hardware')

prof('PPP', [r'^0-0:25\.3\.0\.255$'],
     'target PPP config + nd_ip_port',
     'PHY_reference, LCP options, IPCP options, PPP authentication',
     'target GPRS; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_ppp_get -> target PPP config',
     'SET config -> EEPROM',
     'none',
     'No PPP stack',
     'new target PPP config',
     'EEPROM',
     'No PPP stack')

prof('GPRS', [r'^0-0:25\.4\.0\.255$'],
     'target GPRS config + nd_ip_port',
     'APN, PIN, QoS',
     'target GPRS; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_gprs_get -> target GPRS config',
     'SET config -> EEPROM',
     'none',
     'No GPRS modem',
     'new target GPRS config',
     'EEPROM',
     'No GPRS hardware')

prof('GSM', [r'^0-0:25\.6\.0\.255$'],
     'target GSM diagnostic + nd_ip_port',
     'operator, status, cs/ps, cell_info, adjacent_cells, capture_time',
     'target GPRS modem',
     'nd_cosem_port.get -> nd_ic_gsm_get -> target modem diagnostic',
     'read-only',
     'none',
     'No modem driver',
     'new target GSM diagnostic',
     'none',
     'No GSM hardware')

prof('PUSH', [r'^0-0:25\.9\.', r'^0-[1-7]:25\.9\.'],
     'target push engine',
     'nd_push_setup_t: push_object_list, destination, window, randomisation, retries, repetition_delay, port_reference, SAP, protection, gateway_prefix',
     'target push engine; nd_cosem_port; nd_dlms; nd_ip_port',
     'nd_cosem_port.get -> nd_ic_push_get -> target push config',
     'SET config -> validate -> EEPROM',
     'ACTION push -> read capture objects via port -> nd_dlms data-notification -> nd_ip_port send',
     'No push engine; no Data Notification',
     'new target push engine; reuse nd_cosem_port and nd_ip_port',
     'EEPROM',
     'Push object, destination encoding, windows/retries, Data Notification absent')

prof('G3_COUNTERS', [r'^0-0:29\.0\.'],
     'target G3 MAC counters',
     'uint64 counters for Tx/Rx data/cmd, CSMA failures, CRC, broadcast',
     'target G3 stack',
     'nd_cosem_port.get -> nd_ic_g3_counters_get -> target G3 counters',
     'read-only',
     'ACTION {member} -> clear counters',
     'No G3 stack/hardware',
     'new target G3 counter adapter',
     'none',
     'No G3 hardware/stack')

prof('G3_MAC', [r'^0-0:29\.1\.'],
     'target G3 MAC config',
     'short address, coordinator, PAN id, key table, frame counter, tone mask, TTL, retries, neighbor table, CSMA params',
     'target G3 stack; EEPROM',
     'nd_cosem_port.get -> nd_ic_g3_mac_get -> target G3 config',
     'SET config -> EEPROM',
     'ACTION {member} -> target G3 stack',
     'No G3 stack/hardware',
     'new target G3 MAC config',
     'EEPROM',
     'No G3 hardware/stack')

prof('G3_ADP', [r'^0-0:29\.2\.'],
     'target G3 6LoWPAN config',
     'adp max hops, LQI, security, prefix/routing/broadcast/context/blacklist/group tables, timers, keys',
     'target G3 stack; EEPROM',
     'nd_cosem_port.get -> nd_ic_g3_adp_get -> target G3 config',
     'SET config -> EEPROM',
     'none',
     'No G3 stack/hardware',
     'new target G3 ADP config',
     'EEPROM',
     'No G3 hardware/stack')

prof('ASSOCIATION', [r'^0-0:40\.0\.0\.255$'],
     'components/protocol/dlms association + target association class',
     'nd_dlms_assoc_t (SAP, state, auth, policy, suite, PDU, invoke-id, security ctx); target object_list/secret/users',
     'components/protocol/dlms; target cosem; EEPROM',
     'nd_cosem_port.get -> nd_ic_association_get -> association state',
     'SET secret/user list -> EEPROM',
     'ACTION {member} -> nd_association_{member} -> nd_dlms_assoc + nd_crypto_port',
     'application/adapters/dlms staging only; no runtime DLMS association',
     'new DLMS component and target association class; reuse nd_crypto_port',
     'EEPROM',
     'No runtime DLMS association/SAP')

prof('SAP', [r'^0-0:41\.0\.0\.255$'],
     'components/protocol/dlms association + target SAP table',
     'SAP_assignment_list; nd_dlms SAP table',
     'components/protocol/dlms; EEPROM',
     'nd_cosem_port.get -> nd_ic_sap_get -> target SAP table',
     'SET list -> EEPROM',
     'ACTION connect_logical_device -> nd_dlms_assoc',
     'No runtime DLMS; SAP table absent',
     'new target SAP table; reuse association state',
     'EEPROM',
     'SAP table and connect method absent')

prof('LDN', [r'^0-0:42\.0\.0\.255$'],
     'target product config',
     '16-byte visible string logical device name',
     'target product config; EEPROM',
     'nd_cosem_port.get -> nd_ic_data_get -> target product config',
     'read-only',
     'none',
     'No IDIS product config store',
     'new target product config record',
     'EEPROM',
     'No LDN record')

prof('SECURITY_SETUP', [r'^0-0:43\.0\.'],
     'components/protocol/dlms security + target security class',
     'nd_dlms_security_ctx_t (policy, suite, titles, GUEK/GUBK/GAK, master, frame counters, HLS secret); target key store',
     'components/protocol/dlms; infra/security/crypto; ESAM; EEPROM',
     'nd_cosem_port.get -> nd_ic_security_setup_get -> security ctx',
     'SET policy/suite/titles -> EEPROM/ESAM',
     'ACTION {member} -> nd_security_{member} -> nd_dlms_security + nd_crypto_port',
     'infra/security/crypto; targets/stategrid_meter/security/esam_pro.c; no DLMS security setup class',
     'new target security setup class; reuse crypto/ESAM',
     'EEPROM/ESAM',
     'No DLMS security association, key transfer, certificates')

prof('SECURITY_COUNTER', [r'^0-0:43\.1\.'],
     'components/protocol/dlms security + target persistence',
     'uint32_t frame counters (TX, RX unicast, RX broadcast)',
     'components/protocol/dlms; EEPROM',
     'nd_cosem_port.get -> nd_ic_data_get -> security ctx counter',
     'read-only',
     'none',
     'No DLMS security runtime',
     'reuse security ctx; add counter persistence',
     'EEPROM',
     'No DLMS security runtime')

prof('IMAGE', [r'^0-0:44\.0\.0\.255$'],
     'target image transfer',
     'image_block_size, block status bitstring, first missing block, transfer enabled/status, activate info',
     'target upgrade; SPI flash; bootloader',
     'nd_cosem_port.get -> nd_ic_image_get -> target image state',
     'SET transfer_enabled/info -> flash',
     'ACTION {member} -> nd_image_{member} -> target flash + signature + bootloader',
     'targets/stategrid_meter/upgrade/upgrade.c; infra/boot/Updata',
     'new image transfer class/state; reuse flash/bootloader/signature',
     'SPI flash',
     'No IDIS image transfer object; deferred')

prof('FUNCTION_CONTROL', [r'^0-0:44\.1\.0\.255$', r'^0-1:44\.1\.0\.255$'],
     'target function-control state',
     'activation_status array; function_list (LPCAP_1/2, M-Bus); function specs',
     'domain/freeze; domain/metering; target profile capture',
     'nd_cosem_port.get -> nd_ic_function_control_get -> target flags',
     'SET function_list -> EEPROM',
     'ACTION {member} -> update capture gates',
     'No function-control state',
     'new target function control; reuse profile capture gate',
     'EEPROM',
     'Function list/activation status and capture gates absent')

prof('DEVICE_ID', [r'^0-0:96\.1\.'],
     'target product config',
     'six device ID octet strings/visible strings',
     'target product config; EEPROM',
     'nd_cosem_port.get -> nd_ic_data_get -> product config',
     'read-only',
     'none',
     'No IDIS device ID record',
     'new target product config record',
     'EEPROM',
     'No device ID manufacturing record')

prof('PROFILE_STATUS', [r'^0-0:96\.10\.', r'^0-[1-4]:96\.10\.3\.', r'^1-0:96\.10\.1\.'],
     'target profile buffer',
     'profile status byte (PDN/CAD/DST/DNV/CIV/ERR) from nd_cosem_profile_t header',
     'target profile store; domain/freeze',
     'nd_cosem_port.get -> nd_ic_data_get -> profile header status',
     'read-only',
     'none',
     'No IDIS profile store; legacy freeze status only',
     'new target profile status view',
     'SPI flash header',
     'Profile status/header format not implemented')

prof('EVENT_OBJECT', [r'^0-0:96\.11\.', r'^0-[1-4]:96\.11\.4\.'],
     'target event engine + domain/events',
     'uint8_t event_code (default 255) per log',
     'domain/events; target event engine',
     'nd_cosem_port.get -> nd_ic_data_get -> target event code',
     'read-only',
     'none',
     'domain/events primitives; no IDIS event code mapping',
     'new target event engine; reuse event detectors',
     'none/EEPROM',
     'No event code mapping or event object state')

prof('CONSUMER_MSG', [r'^0-0:96\.13\.'],
     'target display/CIP state',
     'consumer message text/code octet strings',
     'target display/CIP',
     'nd_cosem_port.get -> nd_ic_data_get -> target message state',
     'read-only',
     'none',
     'No display/CIP model',
     'optional target display/CIP state',
     'EEPROM',
     'Optional; no display/CIP model')

prof('ACTIVE_TARIFF', [r'^0-0:96\.14\.0\.255$'],
     'target tariff state + domain/tariff',
     'uint8_t active fee/tariff',
     'domain/tariff; target tariff_lookup',
     'nd_cosem_port.get -> nd_ic_data_get -> tariff_lookup_fee -> fee',
     'read-only',
     'none',
     'domain/tariff/tariff.h:55; targets/stategrid_meter/app/tariff_lookup.c:21',
     'reuse tariff lookup; add target active-tariff cache',
     'none',
     'No IDIS active tariff cache/wiring')

prof('EVENT_COUNTER', [r'^0-0:96\.15\.'],
     'target event engine',
     'uint32_t counter[11/12] by event category (global/billing/LP/M-Bus)',
     'domain/events; target event engine',
     'nd_cosem_port.get -> nd_ic_data_get -> counter',
     'read-only',
     'none',
     'No event counter state',
     'new target event counters; persist',
     'EEPROM',
     'No event counters')

prof('DISCONNECT', [r'^0-0:96\.3\.10\.255$', r'^0-[1-4]:96\.3\.10\.255$', r'^0-[1-4]:24\.4\.0\.255$'],
     'domain/load_control + target disconnect class',
     'load_control_state_t (command, relay_true, timers); IDIS output_state/control_state/control_mode',
     'domain/load_control; target relay adapter; board relay',
     'nd_cosem_port.get -> nd_ic_disconnect_get -> load_control/relay state',
     'SET control_mode -> target state',
     'ACTION {member} -> nd_disconnect_{member} -> load_control_set_command -> relay adapter',
     'domain/load_control/load_control.h:61,80; targets/stategrid_meter/app/relay_control_adapter.c:35; board_io.h:7-9',
     'reuse load_control and relay adapter; add IC70 state mapping',
     'EEPROM',
     'IC70 output/control state mapping, remote methods, mode semantics missing')

prof('POWER_FAILURE', [r'^0-0:96\.7\.'],
     'domain/events + target counters',
     'nd_power_off_state_t (debounce, accumulated seconds); uint16 counters/duration',
     'domain/events; target event engine',
     'nd_cosem_port.get -> nd_ic_data/register_get -> power-failure state',
     'SET threshold -> EEPROM (for threshold object)',
     'none',
     'domain/events/power_off.h:22; no IDIS counters',
     'new target power-failure counters; reuse power_off detector',
     'EEPROM',
     'Long/short power failure classification and counters absent')

prof('ERROR_REGISTER', [r'^0-0:97\.97\.0\.255$'],
     'domain/events + target status',
     'uint32_t error bitfield from event/status states',
     'domain/events; target status',
     'nd_cosem_port.get -> nd_ic_data_get -> error bitfield',
     'read-only',
     'none',
     'Legacy event status in mbase control block; no IDIS error register',
     'new target error register mapping',
     'EEPROM',
     'Bit mapping incomplete')

prof('ALARM_REGISTER', [r'^0-0:97\.98\.(0|1)\.255$'],
     'target alarm engine',
     'uint32_t alarm_bits[2]',
     'domain/events; target alarm engine',
     'nd_cosem_port.get -> nd_ic_data_get -> alarm bits',
     'read-only',
     'none',
     'No IDIS alarm register',
     'new alarm register; map event codes',
     'EEPROM',
     'No alarm register/bit mapping')

prof('ALARM_FILTER', [r'^0-0:97\.98\.(10|11)\.255$'],
     'target alarm engine',
     'uint32_t filter_mask[2]',
     'target alarm engine; EEPROM',
     'nd_cosem_port.get -> nd_ic_data_get -> filter mask',
     'SET filter -> EEPROM',
     'none',
     'No IDIS alarm filter',
     'new alarm filter state',
     'EEPROM',
     'No alarm filter')

prof('ALARM_DESCRIPTOR', [r'^0-0:97\.98\.(20|21)\.255$'],
     'target alarm engine',
     'array of capture object definitions (class_id, OBIS, attr, index)',
     'target alarm engine; EEPROM',
     'nd_cosem_port.get -> nd_ic_data_get -> descriptor array',
     'SET descriptor -> EEPROM',
     'none',
     'No IDIS alarm descriptor',
     'new alarm descriptor state',
     'EEPROM',
     'No alarm descriptor/capture mapping')

prof('BILLING_PROFILE', [r'^0-0:98\.1\.0\.255$'],
     'target profile store + domain/freeze',
     'nd_cosem_profile_t: profile_entries 13, capture objects, ring buffer, status/time header',
     'domain/freeze; domain/metering; domain/tariff; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> profile store -> capture fields',
     'SET capture_objects/period -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset/get_buffer',
     'domain/freeze/schedule.h:34,39,43; legacy gs_freeze.c; no IDIS Profile Generic',
     'new target profile store and class; reuse freeze schedule and nd_store_t',
     'SPI flash',
     'IDIS profile format, capture dispatcher, selective access, compression absent')

prof('EVENT_LOG', [r'^0-0:99\.98\.', r'^1-0:99\.97\.0\.255$'],
     'target profile store + domain/events',
     'nd_cosem_profile_t: event log ring, 100 entries, capture objects (event code/time/etc.)',
     'domain/events; target event engine; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> event log ring',
     'SET capture_objects -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset',
     'domain/events primitives; legacy gs_record.c; no IDIS event log',
     'new event log profile; reuse event detectors',
     'SPI flash',
     'IDIS event log format/capture mapping absent')

prof('MBUS_CLIENT', [r'^0-[1-4]:24\.1\.0\.255$'],
     'target M-Bus client + components/protocol/mbus',
     'nd_mbus_client_t: port ref, capture definition/period, primary address, ID, manufacturer, version, device type, access number, status, alarm, configuration, key status',
     'components/protocol/mbus; target M-Bus; EEPROM',
     'nd_cosem_port.get -> nd_ic_mbus_client_get -> M-Bus client state',
     'SET config -> EEPROM',
     'ACTION {member} -> nd_mbus_client_{member} -> nd_mbus_link',
     'No M-Bus component/hardware',
     'new M-Bus client class and link; reuse UART/board',
     'EEPROM',
     'No M-Bus hardware/protocol; 4 channels')

prof('MBUS_VALUE', [r'^0-[1-4]:24\.2\.'],
     'target M-Bus client + domain/metering',
     'nd_mbus_value_t: value, scaler_unit, status, capture_time; VIF/VIFE mapping',
     'components/protocol/mbus; domain/metering; target M-Bus',
     'nd_cosem_port.get -> nd_ic_extended_register_get -> M-Bus value state',
     'read-only',
     'ACTION reset -> clear value state',
     'No M-Bus component/hardware',
     'new M-Bus value class; reuse extended register semantics',
     'EEPROM/SPI',
     'No M-Bus values; VIF/VIFE conversion absent')

prof('MBUS_PROFILE', [r'^0-[1-4]:24\.3\.0\.255$'],
     'target M-Bus profile + components/protocol/mbus',
     'nd_cosem_profile_t: M-Bus load profile ring, capture objects',
     'components/protocol/mbus; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> M-Bus profile ring',
     'SET capture_objects/period -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset',
     'No M-Bus component/hardware',
     'new M-Bus profile class; reuse profile store',
     'SPI flash',
     'No M-Bus profile storage')

prof('MBUS_CONTROL_LOG', [r'^0-[1-4]:24\.5\.0\.255$'],
     'target M-Bus profile + components/protocol/mbus',
     'nd_cosem_profile_t: control log ring, capture objects',
     'components/protocol/mbus; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> control log ring',
     'SET capture_objects/period -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset',
     'No M-Bus component/hardware',
     'new M-Bus control log class; reuse profile store',
     'SPI flash',
     'No M-Bus control log')

prof('MBUS_DEVICE_ID', [r'^0-[1-4]:96\.1\.'],
     'target M-Bus client',
     'device ID octet strings from M-Bus client state',
     'components/protocol/mbus; target M-Bus',
     'nd_cosem_port.get -> nd_ic_data_get -> M-Bus client ID',
     'read-only',
     'none',
     'No M-Bus component/hardware',
     'new M-Bus device ID view',
     'EEPROM',
     'No M-Bus hardware/protocol')

prof('ENERGY', [r'^1-0:(15|16)\.8\.', r'^1-0:(1|2|3|4|5|6|7|8|9|10)\.8\.'],
     'domain/metering + target register class',
     'nd_energy_book_t.forward/reverse.slots[13]: nd_energy_amount_t {integer hundredths kWh, fraction pulses}; meter_const/cal_word',
     'domain/metering; target persistence; board RN8209',
     'nd_cosem_port.get -> nd_ic_register_get -> metering_read/metering_read_combined -> int64 -> scaler_unit',
     'read-only; method reset -> metering_clear',
     'ACTION reset -> metering_clear -> persistence',
     'domain/metering/metering.h:38,42,45; energy.h:57-66; targets/stategrid_meter/app/metering_energy_adapter.c:133; persistence/legacy_energy_store.c:9',
     'reuse domain and energy codec; create target-owned metering_state_t; do not include legacy control block',
     'EEPROM energy slice / new IDIS record',
     'Combined/phase mapping and IDIS scaler units need table; legacy adapter imports mbase control block')

prof('INSTANTANEOUS', [r'^1-0:(1|2|3|4|9|10|15|16)\.7\.0\.255$', r'^1-0:(31|32|51|52|71|72|90)\.7\.0\.255$', r'^1-0:13\.7\.0\.255$', r'^1-0:14\.7\.0\.255$'],
     'target measurement adapter + board RN8209',
     'legacy mbase_control_block_t fields voltage_ac, current_l, active/apparent_power, power_factor, frequency, current_n; IDIS needs per-phase value/scaler_unit',
     'domain/metering measurement (new); board RN8209',
     'nd_cosem_port.get -> nd_ic_register_get -> target measurement adapter -> RN8209 registers -> value/scaler_unit',
     'read-only',
     'none',
     'targets/stategrid_meter/app/metering/electrical_measurement.c:988; board/ndv_RN8209.c; no domain API',
     'new domain measurement value object/port; reuse RN8209 driver; 105 needs phase arrays',
     'none',
     'No domain measurement API; single-phase only; no L2/L3; signed/quadrant/scaler mapping incomplete')

prof('DEMAND', [r'^1-0:(1|2|3|4|9|10)\.4\.0\.255$'],
     'domain/metering/demand + target demand class',
     'nd_demand_t opaque: config, cumulative_seconds, slides[], value, fee_value; no last/status/time',
     'domain/metering/demand; application/metering/demand_registry; target demand adapter',
     'nd_cosem_port.get -> nd_ic_demand_register_get -> nd_demand_value/nd_demand_fee_value',
     'read-only',
     'ACTION {member} -> nd_demand_reset (missing) / nd_demand_on_minute',
     'domain/metering/demand.h:30-41; demand.c:71-126; application/metering/demand_registry.h:21',
     'reuse demand engine; extend snapshot API for last/status/time/period; target demand registry',
     'EEPROM/SPI',
     'No last average/status/capture time/period API; reset/next_period semantics missing')

prof('LAST_DEMAND', [r'^1-0:(1|2|3|4|9|10)\.5\.0\.255$'],
     'domain/metering/demand + target demand class',
     'nd_demand_t current value only; last average requires a second snapshot/history',
     'domain/metering/demand; target demand adapter',
     'nd_cosem_port.get -> nd_ic_extended_register_get -> last-demand snapshot',
     'read-only',
     'ACTION {member} -> clear last snapshot',
     'domain/metering/demand.h:30-41; no last snapshot',
     'extend nd_demand_t with previous value/capture time; target adapter',
     'EEPROM/SPI',
     'Last average value/capture_time not stored')

prof('MAX_DEMAND', [r'^1-0:(1|2|3|4|9|10)\.6\.'],
     'domain/metering/demand + target demand class',
     'max value + capture_time + status; nd_demand_t does not expose history',
     'domain/metering/demand; target demand adapter; freeze',
     'nd_cosem_port.get -> nd_ic_extended_register_get -> max-demand state',
     'read-only',
     'ACTION {member} -> clear max state',
     'domain/metering/demand.h:30-41; application/metering/demand_registry.h:21; no max state',
     'new max-demand snapshot in target; reuse demand engine',
     'EEPROM/SPI',
     'Max value/capture_time/status not stored')

prof('AVG_MEASUREMENT', [r'^1-0:(1|2|9|10|15|16|31|32|51|52|71|72|90)\.24\.0\.255$'],
     'target measurement adapter',
     'legacy active_avg_power / per-phase average arrays; IDIS average value/scaler_unit',
     'domain/metering measurement (new); board RN8209',
     'nd_cosem_port.get -> nd_ic_register_get -> target measurement average state',
     'read-only',
     'none',
     'electrical_measurement.c:1061-1077 computes active_avg_power only',
     'new per-phase average state; reuse measurement adapter',
     'none',
     'Only active power average; no voltage/current/average per phase')

prof('PQ_THRESHOLD', [r'^1-0:12\.'],
     'target PQ detector config',
     'voltage sag/swell/cut thresholds and time thresholds',
     'domain/events (new PQ detector); EEPROM',
     'nd_cosem_port.get -> nd_ic_register_get -> target PQ config',
     'SET threshold -> EEPROM',
     'none',
     'No PQ thresholds/detector',
     'new target PQ config and detector',
     'EEPROM',
     'No PQ detector/threshold state')

prof('PQ_EVENT', [r'^1-0:(32|52|72)\.(32|33|34|36|37|38)\.0\.255$'],
     'target PQ detector + domain/events',
     'per-phase counters, last duration/magnitude for sag/swell',
     'domain/events; target PQ detector; event log',
     'nd_cosem_port.get -> nd_ic_data/register_get -> PQ event state',
     'read-only (threshold object separate)',
     'none',
     'No PQ detector/counters',
     'new PQ detector; reuse event duration primitives',
     'EEPROM/SPI',
     'No PQ event counters/duration/magnitude')

prof('FUSE', [r'^1-0:(31|51|71)\.4\.0\.255$'],
     'domain/events + domain/metering/demand (conflict)',
     'IC21 monitor vs IC5 sliding average share one OBIS; cannot choose before conflict resolution',
     'domain/events; domain/metering/demand; load_control',
     'nd_cosem_port.get/action -> class unknown until C1 resolved',
     'unknown until C1 resolved',
     'unknown until C1 resolved',
     'domain/events/over_current.h:18; demand.h:30-41; conflict C1',
     'blocked; resolve IC21 vs IC5 with Hexing/IDIS',
     'EEPROM',
     'Hard blocker C1; no class assignment')

prof('LOAD_PROFILE', [r'^1-0:99\.(1|2)\.0\.255$'],
     'target profile store + domain/metering + domain/freeze',
     'nd_cosem_profile_t: LP1 960 entries, LP2 240; capture objects energy/instantaneous',
     'domain/metering; domain/freeze; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> profile ring',
     'SET capture_objects/period -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset',
     'domain/freeze/schedule.h:34,39; legacy gs_freeze.c; no IDIS profile',
     'new profile store/class; reuse freeze due and metering',
     'SPI flash',
     'IDIS profile format/capture dispatcher/selective access absent')

prof('PQ_PROFILE', [r'^1-0:99\.14\.0\.255$'],
     'target profile store + domain/events',
     'nd_cosem_profile_t: PQ profile 1152 entries, 12 capture objects',
     'domain/events; target profile store',
     'nd_cosem_port.get -> nd_ic_profile_get -> PQ ring',
     'SET capture_objects/period -> EEPROM',
     'ACTION {member} -> nd_profile_capture/reset',
     'No PQ profile store',
     'new PQ profile; reuse profile store',
     'SPI flash',
     'No PQ profile/storage')

prof('FIRMWARE', [r'^1-[0-2]:0\.2\.'],
     'target upgrade/bootloader',
     'firmware identifier/signature from flash/bootloader',
     'target upgrade; SPI flash',
     'nd_cosem_port.get -> nd_ic_data_get -> flash/bootloader version',
     'read-only',
     'none',
     'targets/stategrid_meter/upgrade/upgrade.c; infra/boot/Updata',
     'reuse version/signature access; new IDIS data object',
     'SPI flash',
     'Firmware version/signature mapping absent')

prof('METER_CONST', [r'^1-0:0\.8\.2\.255$'],
     'domain/metering',
     'nd_energy_book_t.meter_const / metering_config_t.meter_const',
     'domain/metering; EEPROM',
     'nd_cosem_port.get -> nd_ic_register_get -> metering config',
     'SET constant -> metering_configure -> EEPROM',
     'none',
     'domain/metering/energy.h:37; metering.h:16; metering_energy_adapter.c:26',
     'reuse metering config; add IDIS writable parameter',
     'EEPROM',
     'No writable meter constant object')

prof('POWER_FACTOR', [r'^1-0:13\.(3|5)\.0\.255$'],
     'target measurement adapter',
     'legacy power_factor int32 (milli); min/last average snapshots',
     'domain/metering measurement (new); board RN8209',
     'nd_cosem_port.get -> nd_ic_extended_register_get -> measurement state',
     'read-only',
     'ACTION reset -> clear min/max history',
     'electrical_measurement.c:1124-1138; no min/last state',
     'new measurement history; reuse RN8209',
     'EEPROM',
     'No min/last power-factor state')

prof('FREQUENCY', [r'^1-0:14\.7\.0\.255$'],
     'target measurement adapter',
     'legacy frequency uint32 (0.01 Hz)',
     'domain/metering measurement (new); board RN8209',
     'nd_cosem_port.get -> nd_ic_register_get -> measurement frequency',
     'read-only',
     'none',
     'electrical_measurement.c:1104-1107',
     'reuse measurement adapter; expose domain value',
     'none',
     'No domain measurement API')

def profile_for(ob):
    for p in P:
        for pat in p['patterns']:
            if re.match(pat, ob):
                return p
    return None

unmatched = []
for r in ROWS:
    if profile_for(r['obis']) is None and r['obis'] not in unmatched:
        unmatched.append(r['obis'])
if unmatched:
    print('unmatched', len(unmatched))
    for ob in unmatched:
        print(ob)

CLASS = {
    '1': 'nd_ic_data', '3': 'nd_ic_register', '4': 'nd_ic_extended_register',
    '5': 'nd_ic_demand_register', '6': 'nd_ic_register_activation',
    '7': 'nd_ic_profile', '8': 'nd_ic_clock', '9': 'nd_ic_script',
    '11': 'nd_ic_special_days', '15': 'nd_ic_association', '17': 'nd_ic_sap',
    '18': 'nd_ic_image', '19': 'nd_ic_local_port',
    '20': 'nd_ic_activity_calendar', '21': 'nd_ic_register_monitor',
    '22': 'nd_ic_single_action', '23': 'nd_ic_hdlc_setup',
    '27': 'nd_ic_modem', '28': 'nd_ic_auto_answer', '29': 'nd_ic_auto_connect',
    '40': 'nd_ic_push', '41': 'nd_ic_tcp_udp', '42': 'nd_ic_ipv4',
    '43': 'nd_ic_mac', '44': 'nd_ic_ppp', '45': 'nd_ic_gprs',
    '47': 'nd_ic_gsm', '48': 'nd_ic_ipv6', '64': 'nd_ic_security_setup',
    '70': 'nd_ic_disconnect', '71': 'nd_ic_limiter', '72': 'nd_ic_mbus_client',
    '74': 'nd_ic_mbus_port', '90': 'nd_ic_g3_counters',
    '91': 'nd_ic_g3_mac', '92': 'nd_ic_g3_adp', '100': 'nd_ic_ntp',
    '122': 'nd_ic_function_control',
}

def has_set(r):
    return 'Set' in (r['public'] + r['preest'] + r['mgmt'])


MEMBER_OVERRIDES = {
    ('8', 'time'): {
        'data': 'nd_datetime_t; nd_dlms_datetime_t 12-byte wire form',
        'read': 'target clock adapter -> nd_datetime_t -> nd_axdr date-time[12]',
        'write': 'SET -> nd_calendar_date_valid/time_valid -> target RTC set',
        'gap': 'No clock port; DST/timezone not applied to local time',
    },
    ('8', 'time_zone'): {
        'data': 'int16_t minutes from UTC',
        'read': 'target clock config -> int16_t -> A-XDR long',
        'write': 'SET -> validate range -> EEPROM',
        'gap': 'No timezone record; DST rules absent',
    },
    ('8', 'status'): {
        'data': 'uint8_t clock status bitfield',
        'read': 'target RTC -> clock status -> A-XDR unsigned',
        'write': 'read-only',
        'gap': 'RTC error/invalid status not exposed',
    },
    ('8', 'daylights_savings_begin'): {
        'data': 'nd_datetime_t DST begin',
        'read': 'target clock config -> nd_datetime_t -> date-time[12]',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'DST begin not stored',
    },
    ('8', 'daylights_savings_end'): {
        'data': 'nd_datetime_t DST end',
        'read': 'target clock config -> nd_datetime_t -> date-time[12]',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'DST end not stored',
    },
    ('8', 'daylights_savings_deviation'): {
        'data': 'int8_t DST deviation in minutes',
        'read': 'target clock config -> int8_t -> A-XDR integer',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'DST deviation not stored',
    },
    ('8', 'daylights_savings_enabled'): {
        'data': 'bool DST enabled',
        'read': 'target clock config -> bool',
        'write': 'SET -> EEPROM',
        'gap': 'DST enable flag not stored',
    },
    ('8', 'clock_base'): {
        'data': 'enum clock_base (local/UTC)',
        'read': 'target clock config -> enum',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Clock base not stored',
    },
    ('4', 'status'): {
        'data': 'uint8_t status bitfield (PDN/CAD/DST/DNV/CIV/ERR)',
        'read': 'domain state -> status byte -> A-XDR unsigned',
        'write': 'read-only',
        'gap': 'Status semantics per register family not defined',
    },
    ('4', 'capture_time'): {
        'data': 'nd_datetime_t capture_time',
        'read': 'domain state -> nd_datetime_t -> date-time[12]',
        'write': 'read-only',
        'gap': 'Capture time not stored for demand/max-demand',
    },
    ('5', 'current_average_value'): {
        'data': 'uint32_t current average (nd_demand_t.value/fee_value)',
        'read': 'nd_demand_value/nd_demand_fee_value -> uint32_t -> A-XDR',
        'write': 'read-only',
        'gap': 'Current average only; no last/status/time API',
    },
    ('5', 'last_average_value'): {
        'data': 'uint32_t previous average snapshot',
        'read': 'target demand snapshot -> uint32_t',
        'write': 'read-only',
        'gap': 'Previous value not stored in nd_demand_t',
    },
    ('5', 'status'): {
        'data': 'uint8_t demand status',
        'read': 'target demand snapshot -> uint8_t',
        'write': 'read-only',
        'gap': 'Status not stored',
    },
    ('5', 'capture_time'): {
        'data': 'nd_datetime_t demand capture time',
        'read': 'target demand snapshot -> nd_datetime_t',
        'write': 'read-only',
        'gap': 'Capture time not stored',
    },
    ('5', 'start_time_current'): {
        'data': 'nd_datetime_t period start',
        'read': 'target demand snapshot -> nd_datetime_t',
        'write': 'read-only',
        'gap': 'Period start not stored',
    },
    ('5', 'period'): {
        'data': 'uint32_t demand period seconds (new field)',
        'read': 'target demand config -> period -> A-XDR',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No period field or setter',
    },
    ('5', 'number_of_periods'): {
        'data': 'uint16_t number of periods',
        'read': 'nd_demand_config_t.slide_num/max_slide -> uint16_t',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No setter/validation API',
    },
    ('7', 'buffer'): {
        'data': 'nd_cosem_profile_t ring buffer (header + encoded records)',
        'read': 'profile store -> selective access (range/index) -> A-XDR array',
        'write': 'read-only; reset/capture methods mutate',
        'gap': 'IDIS record format/selective access not implemented',
    },
    ('7', 'capture_objects'): {
        'data': 'nd_capture_object_t[] (class_id, OBIS, attr, data_index)',
        'read': 'profile config -> array of capture_object_definition',
        'write': 'SET -> validate against nd_idis_ids.h -> EEPROM',
        'gap': 'No capture-object parser/validator',
    },
    ('7', 'capture_period'): {
        'data': 'uint32_t seconds; 0/300/600/900/1800/3600/86400',
        'read': 'profile config -> uint32_t',
        'write': 'SET -> validate allowed period -> EEPROM',
        'gap': 'Period validation not implemented',
    },
    ('7', 'sort_method'): {
        'data': 'enum sort_method (1/4)',
        'read': 'profile config -> enum',
        'write': 'SET -> validate 1/4 -> EEPROM',
        'gap': 'Sorting not implemented',
    },
    ('7', 'sort_object'): {
        'data': 'nd_capture_object_t sort_object',
        'read': 'profile config -> capture_object_definition',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Sort object validation not implemented',
    },
    ('7', 'entries_in_use'): {
        'data': 'uint16_t entries_in_use',
        'read': 'profile header -> uint16_t',
        'write': 'read-only',
        'gap': 'Ring header not implemented',
    },
    ('7', 'profile_entries'): {
        'data': 'uint16_t capacity (13/240/960/1152/100/30/10)',
        'read': 'profile config -> uint16_t',
        'write': 'SET -> fixed per profile -> EEPROM',
        'gap': 'Capacity table not encoded',
    },
    ('6', 'register_assignment'): {
        'data': 'register_assignment_item[] (class_id, OBIS, attr)',
        'read': 'target activation state -> array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Activation state absent',
    },
    ('6', 'mask_list'): {
        'data': 'array of mask entries',
        'read': 'target activation state -> array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Mask state absent',
    },
    ('6', 'active_mask'): {
        'data': 'octet_string[0..8] active mask',
        'read': 'target activation state -> octets',
        'write': 'read-only; add/remove mask methods mutate',
        'gap': 'Mask methods/storage absent',
    },
    ('9', 'scripts'): {
        'data': 'nd_script_t[] (script_id, actions[])',
        'read': 'target script table -> array of script9',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Script table storage absent',
    },
    ('11', 'entries'): {
        'data': 'nd_special_day_t[] (day_id, month, day)',
        'read': 'target calendar -> array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'IDIS special-days model absent; max 30',
    },
    ('20', 'season_profile_active'): {
        'data': 'season_profile[] (season, start)',
        'read': 'target calendar -> active season array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Full calendar model absent',
    },
    ('20', 'week_profile_table_active'): {
        'data': 'week_profile_table[] (week id, day schedule)',
        'read': 'target calendar -> active week array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Week profile model absent',
    },
    ('20', 'day_profile_table_active'): {
        'data': 'day_profile_table[] (day id, periods)',
        'read': 'target calendar -> active day array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Day profile model absent',
    },
    ('40', 'push_object_list'): {
        'data': 'nd_capture_object_t[] push capture list',
        'read': 'push config -> array of capture_object_definition',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'Push object absent',
    },
    ('40', 'send_destination_and_method'): {
        'data': 'nd_push_destination_t (transport, address, method)',
        'read': 'push config -> structure',
        'write': 'SET -> encode destination -> EEPROM',
        'gap': 'Destination encoding absent',
    },
    ('70', 'output_state'): {
        'data': 'bool output_state from relay feedback',
        'read': 'load_control/relay status -> bool',
        'write': 'read-only',
        'gap': 'IC70 state mapping absent',
    },
    ('70', 'control_state'): {
        'data': 'enum control_state (connected/ready/disconnected)',
        'read': 'load_control command/state -> enum',
        'write': 'SET -> load_control_set_command',
        'gap': 'IC70 state mapping absent',
    },
    ('70', 'control_mode'): {
        'data': 'enum control_mode',
        'read': 'target disconnect config -> enum',
        'write': 'SET -> EEPROM',
        'gap': 'Control mode semantics absent',
    },
    ('64', 'security_policy'): {
        'data': 'uint8_t security_policy',
        'read': 'nd_dlms_security_ctx_t -> A-XDR enum',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No DLMS security runtime',
    },
    ('64', 'security_suite'): {
        'data': 'uint8_t security_suite',
        'read': 'nd_dlms_security_ctx_t -> enum',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No DLMS security runtime',
    },
    ('15', 'object_list'): {
        'data': 'object_list_type (class_id, version, OBIS, access)',
        'read': 'target association object -> array',
        'write': 'SET via add/remove object methods',
        'gap': 'No DLMS association runtime',
    },
    ('17', 'SAP_assignment_list'): {
        'data': 'asslist_type (SAP, logical device)',
        'read': 'target SAP table -> array',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'SAP table absent',
    },
    ('18', 'image_transferred_block_status'): {
        'data': 'bit_string block status',
        'read': 'target image state -> bit-string',
        'write': 'read-only; block transfer mutates',
        'gap': 'Image transfer object absent',
    },
    ('42', 'IP_address'): {
        'data': 'uint32_t IPv4 address',
        'read': 'target net config -> uint32_t',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No IPv4 stack',
    },
    ('48', 'unicast_IPv6_addresses'): {
        'data': 'IPv6_address[]',
        'read': 'target net config -> array',
        'write': 'SET via add/remove address',
        'gap': 'No IPv6 stack',
    },
    ('72', 'primary_address'): {
        'data': 'uint8_t M-Bus primary address',
        'read': 'nd_mbus_client_t -> uint8_t',
        'write': 'SET -> validate -> EEPROM',
        'gap': 'No M-Bus client state',
    },
    ('72', 'encryption_key_status'): {
        'data': 'enum key status (no key/set/transferred/in use)',
        'read': 'nd_mbus_client_t -> enum',
        'write': 'read-only; key methods mutate',
        'gap': 'No M-Bus security state',
    },
    ('90', 'mac_Tx_data_packet_count'): {
        'data': 'uint64_t MAC counter',
        'read': 'target G3 counters -> uint64_t',
        'write': 'read-only; reset method clears',
        'gap': 'No G3 stack/hardware',
    },
    ('91', 'mac_neighbour_table'): {
        'data': 'neighbour table array',
        'read': 'target G3 MAC -> array',
        'write': 'read-only; get entry method',
        'gap': 'No G3 stack/hardware',
    },
    ('92', 'adp_routing_table'): {
        'data': 'routing table array',
        'read': 'target G3 ADP -> array',
        'write': 'SET -> EEPROM',
        'gap': 'No G3 stack/hardware',
    },
    ('100', 'server_address'): {
        'data': 'octet string NTP server',
        'read': 'target NTP config -> octets',
        'write': 'SET -> EEPROM',
        'gap': 'No NTP client',
    },
    ('122', 'activation_status'): {
        'data': 'array of functional block status',
        'read': 'target function control -> array',
        'write': 'read-only; set_function_status mutates',
        'gap': 'Function control state absent',
    },
}


def apply_override(r, member, p, data, read, write, gap):
    if r['ic'] == '5' and p['id'] != 'DEMAND':
        return data, read, write, gap
    ov = MEMBER_OVERRIDES.get((r['ic'], member))
    if ov is None:
        return data, read, write, gap
    return (ov.get('data', data), ov.get('read', read),
            ov.get('write', write), ov.get('gap', gap))


def build_row(r):
    p = profile_for(r['obis'])
    cls = CLASS.get(r['ic'], 'nd_ic_unknown')
    member = r['member']
    if member == 'logical_name':
        return dict(
            data_structure='nd_obis_t: 6-byte logical name',
            read_chain=f"GET {r['obis']} attr {r['id']} -> nd_cosem_port.get -> {cls}_get -> nd_idis_ids.h symbol -> A-XDR octet-string[6]",
            write_chain='read-only',
            action_chain='',
            state_owner='target mapper_idis',
            domain_deps='none',
            existing_code='targets/idis_ip_meter/mapper_idis/nd_idis_ids.h (planned)',
            embedding='generate symbol from all_obis.csv; class handler returns it',
            persistence='none',
            gap='none; symbol generated by gen_idis_ids.py',
        )
    if member == 'scaler_unit':
        return dict(
            data_structure=p['scaler'] or 'nd_scaler_unit_t (scaler, unit) from domain value',
            read_chain=f"GET {r['obis']} attr {r['id']} -> nd_cosem_port.get -> {cls}_get -> domain scaler_unit -> A-XDR structure",
            write_chain='read-only' if not has_set(r) else f"SET {member} -> {cls}_set -> validate -> parameter store",
            action_chain='',
            state_owner=p['state'],
            domain_deps=p['deps'],
            existing_code=p['existing'],
            embedding=p['embed'],
            persistence=p['persist'],
            gap=p['gap'],
        )
    detail = re.sub(r'^nd_cosem_port\.get -> (?:nd_ic_[a-z_]+_get -> )?', '', p['read'])
    data = p['data']
    rdetail = detail
    wdetail = re.sub(r'^nd_cosem_port\.set -> (?:nd_ic_[a-z_]+_set -> )?', '', p['write'])
    wdetail = re.sub(r'^SET ', '', wdetail)
    gap = p['gap']
    data, rdetail, wdetail, gap = apply_override(r, member, p, data, rdetail, wdetail, gap)
    read = f"GET {r['obis']} attr {r['id']} ({member}) -> nd_cosem_port.get -> {cls}_get -> {rdetail}"
    write = (f"SET {r['obis']} attr {r['id']} ({member}) -> nd_cosem_port.set -> {cls}_set -> {wdetail}"
             if has_set(r) else 'read-only (no SET access in IDIS)')
    action = ''
    if r['kind'] == 'method':
        read = ''
        write = ''
        adetail = re.sub(r'^ACTION ', '', p['action'].replace('{member}', member))
        action = f"ACTION {r['obis']} method {r['id']} ({member}) -> nd_cosem_port.action -> {cls}_action -> {adetail}"
    return dict(
        data_structure=data,
        read_chain=read,
        write_chain=write,
        action_chain=action,
        state_owner=p['state'],
        domain_deps=p['deps'],
        existing_code=p['existing'],
        embedding=p['embed'],
        persistence=p['persist'],
        gap=gap,
    )

PROFILE_BY_ID = {p['id']: p for p in P}

OUT = []
for r in ROWS:
    p = profile_for(r['obis'])
    out = {'obis': r['obis'], 'ic': r['ic'], 'version': r['version'],
           'object': r['object'], 'domain': r['domain'], 'phase': r['phase'],
           'kind': r['kind'], 'id': r['id'], 'member': r['member'],
           'cosem_type': r['type'], 'chain_id': p['id']}
    out.update(build_row(r))
    OUT.append(out)


SUPPORT = {
    'ENERGY': 'reuse', 'METER_CONST': 'reuse', 'ACTIVE_TARIFF': 'reuse',
    'CLOCK': 'partial', 'LOCAL_TIME': 'partial', 'TIME_SHIFT_LIMIT': 'partial',
    'SPECIAL_DAYS': 'partial', 'ACTIVITY_CALENDAR': 'partial',
    'SCHEDULER': 'partial', 'ALARM_MONITOR': 'partial', 'LIMITER': 'partial',
    'DEMAND': 'partial', 'LAST_DEMAND': 'partial', 'MAX_DEMAND': 'partial',
    'INSTANTANEOUS': 'partial', 'AVG_MEASUREMENT': 'partial',
    'DISCONNECT': 'partial', 'POWER_FAILURE': 'partial',
    'BILLING_PROFILE': 'partial', 'EVENT_LOG': 'partial', 'LOAD_PROFILE': 'partial',
    'EVENT_OBJECT': 'partial', 'ERROR_REGISTER': 'partial',
    'PROFILE_STATUS': 'new', 'EVENT_COUNTER': 'new',
    'FUSE': 'blocked',
}
SUPPORT_LABEL = {
    'reuse': 'domain 可复用',
    'partial': 'domain 部分可复用',
    'new': '无现有实现',
    'blocked': '被冲突阻塞',
}


def support(pid):
    return SUPPORT.get(pid, 'new')


for r in OUT:
    r['support'] = support(r['chain_id'])

assert all(r['data_structure'] and r['state_owner'] and r['gap'] for r in OUT)
assert all(r['read_chain'] for r in OUT if r['kind'] == 'attribute')
assert all(r['action_chain'] for r in OUT if r['kind'] == 'method')
assert {r['obis'] for r in OUT} == {r['obis'] for r in ROWS}

FIELDS = ['obis', 'ic', 'version', 'object', 'domain', 'phase', 'kind', 'id',
          'member', 'cosem_type', 'chain_id', 'support', 'state_owner',
          'data_structure', 'read_chain', 'write_chain', 'action_chain',
          'domain_deps', 'existing_code', 'embedding', 'persistence', 'gap']

with open('docs/idis/obis_logic_chains.csv', 'w', newline='', encoding='utf-8') as f:
    w = csv.DictWriter(f, fieldnames=FIELDS)
    w.writeheader()
    w.writerows(OUT)

# Object-level index, grouped by domain.
objects = {}
for r in OUT:
    objects.setdefault(r['obis'], []).append(r)

with open('docs/idis/obis_logic_chains_objects.md', 'w', encoding='utf-8') as f:
    f.write('# IDIS P3 OBIS 对象逻辑链索引\n\n')
    f.write(f'{len(objects)} 个 OBIS，{len(OUT)} 条属性/方法行。'
            '逐成员明细见 `obis_logic_chains.csv`。\n\n')
    f.write('状态说明：domain 可复用 = 现有 domain API 已能支撑；'
            'domain 部分可复用 = 有领域逻辑但缺 IDIS 状态/接口；'
            '无现有实现 = 需要新 domain 或 target 状态；'
            '被冲突阻塞 = 需先解决标准冲突。\n\n')
    current = None
    for ob in sorted(objects, key=lambda o: (objects[o][0]['domain'], o)):
        rs = objects[ob]
        if rs[0]['domain'] != current:
            current = rs[0]['domain']
            f.write(f'\n## {current}\n\n')
            f.write('| OBIS | IC | 对象 | 成员 | 链路模板 | 状态 | 数据/状态所有者 | 主要缺口 |\n')
            f.write('|---|---|---|---|---|---|---|---|\n')
        attrs = sum(1 for r in rs if r['kind'] == 'attribute')
        methods = sum(1 for r in rs if r['kind'] == 'method')
        chains = ','.join(sorted({r['chain_id'] for r in rs}))
        rep = next((r for r in rs if r['member'] != 'logical_name'), rs[0])
        gap = PROFILE_BY_ID[rep['chain_id']]['gap'].replace('|', '/')
        f.write(f"| `{ob}` | {rep['ic']} | {rep['object']} | {attrs}A/{methods}M | "
                f"{chains} | {SUPPORT_LABEL[rep['support']]} | {rep['state_owner']} | {gap} |\n")

print(f'{len(OUT)} rows, {len(objects)} objects -> docs/idis/obis_logic_chains.csv')
print('object index -> docs/idis/obis_logic_chains_objects.md')
