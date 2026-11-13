#!/usr/bin/env python3
"""Expand every OBIS into its attributes and methods.

Objects parsed from the object-model sheet use their own member rows. Objects
that only appear in the extension sheets or the IP profile reuse the member
list of another instance of the same interface class and are marked as such.
"""
import csv
import re
from collections import Counter, defaultdict

OM = 'docs/idis/object_model.csv'
OBIS = 'docs/idis/all_obis.csv'
OUT = 'docs/idis/obis_members.csv'
MD = 'docs/idis/obis_members.md'

PHASE = [
    (r'^0-0:43\.', '4', 'target cosem security setup / nd_dlms_security'),
    (r'^0-0:40\.', '3', 'target cosem association / nd_dlms_assoc'),
    (r'^0-0:41\.', '3', 'target cosem SAP assignment'),
    (r'^0-0:42\.', '6a', 'product config: logical device name'),
    (r'^0-0:96\.1\.', '6a', 'product config: device IDs'),
    (r'^0-0:1\.0\.0\.255$', '6a', 'domain/common/calendar clock API'),
    (r'^1-0:0\.9\.11\.255$', '6a', 'clock config parameter'),
    (r'^0-0:96\.14\.0\.255$', '6a', 'domain/tariff API'),
    (r'^0-0:(13|11|14)\.', '6a', 'domain/tariff + register activation'),
    (r'^0-0:10\.0\.100\.255$', '6a', 'domain/tariff script table'),
    (r'^0-0:10\.0\.1\.255$', '6a', 'domain/freeze billing reset script'),
    (r'^0-0:15\.0\.0\.255$', '6a', 'domain/freeze billing scheduler'),
    (r'^1-0:[1-8]\.8\.', '6a', 'domain/metering energy API'),
    (r'^1-0:(15|16)\.8\.', '6a', 'domain/metering energy API'),
    (r'^1-0:[1-4]\.4\.0\.255$', '6a', 'domain/metering/demand'),
    (r'^1-0:[1-4]\.6\.', '6a', 'domain/metering/demand + freeze'),
    (r'^1-0:(32|52|72|31|51|71|90)\.(7|24)\.', '6a', 'domain/metering instantaneous API'),
    (r'^0-0:98\.1\.0\.255$', '5', 'domain/freeze + target profile'),
    (r'^1-0:99\.(1|2|14)\.0\.255$', '5', 'target profile + metering'),
    (r'^0-0:99\.(98|97)\.', '5', 'target profile event logs'),
    (r'^0-0:96\.10\.', '5', 'profile status'),
    (r'^0-0:97\.(97|98)\.', '6b', 'domain/events + alarm state'),
    (r'^0-0:16\.1\.', '6b', 'alarm monitor'),
    (r'^0-0:96\.11\.', '6b', 'event object'),
    (r'^0-0:96\.15\.', '6b', 'event counter'),
    (r'^1-0:[1-8]\.(32|33|34|36|37|38)\.', '6b', 'domain/events power quality'),
    (r'^0-0:96\.7\.', '6b', 'domain/events power failure'),
    (r'^0-0:25\.9\.', '6c', 'target cosem push setup'),
    (r'^0-0:10\.0\.108\.255$', '6c', 'target cosem push script table'),
    (r'^0-[123]:15\.0\.4\.255$', '6c', 'target cosem push schedulers'),
    (r'^0-4:15\.0\.4\.255$', '6c', 'target cosem push on alarm scheduler'),
    (r'^0-0:44\.1\.0\.255$', '6c', 'target cosem function control'),
    (r'^0-0:15\.0\.7\.255$', '6c', 'target cosem function scheduler'),
    (r'^0-0:10\.0\.111\.255$', '6c', 'target cosem function script table'),
    (r'^0-0:96\.3\.10\.255$', '6c', 'domain/load_control relay API'),
    (r'^0-0:15\.0\.1\.255$', '6c', 'target cosem disconnect scheduler'),
    (r'^0-0:13\.0\.1\.255$', '6c', 'target cosem disconnect calendar'),
    (r'^0-0:11\.0\.1\.255$', '6c', 'target cosem disconnect special days'),
    (r'^0-0:10\.0\.106\.255$', '6c', 'target cosem disconnect script table'),
    (r'^0-0:17\.0\.0\.255$', '6c', 'domain/load_control relay/limiter API'),
    (r'^0-0:10\.0\.103\.255$', '6c', 'target cosem load mgmt script table'),
    (r'^0-[0-9]+:96\.3\.10\.255$', '6c', 'domain/load_control relay API'),
    (r'^0-0:13\.0\.2\.255$', '6c', 'target cosem load mgmt calendar'),
    (r'^0-0:11\.0\.2\.255$', '6c', 'target cosem load mgmt special days'),
    (r'^0-0:44\.0\.0\.255$', '8', 'image transfer'),
    (r'^0-0:15\.0\.2\.255$', '8', 'image activation scheduler'),
    (r'^0-0:10\.0\.107\.255$', '8', 'image activation script'),
    (r'^1-0:0\.2\.', '8', 'firmware version'),
    (r'^0-0:25\.0\.0\.255$', '7', 'TCP-UDP setup, shared by all media'),
    (r'^0-0:25\.1\.0\.255$', '7', 'IPv4 setup, shared by all media'),
    (r'^0-0:25\.7\.0\.255$', '7', 'IPv6 setup, shared by all media'),
    (r'^0-0:25\.2\.0\.255$', '7A/7B', 'MAC address setup, Ethernet/G3'),
    (r'^0-0:2\.1\.0\.255$', '7', 'auto connect, all media'),
    (r'^0-0:2\.0\.0\.255$', '7C', 'modem configuration, GPRS'),
    (r'^0-0:2\.2\.0\.255$', '7C', 'auto answer, GPRS'),
    (r'^0-0:25\.3\.0\.255$', '7C', 'PPP setup, GPRS'),
    (r'^0-0:25\.4\.0\.255$', '7C', 'GPRS modem setup'),
    (r'^0-0:25\.6\.0\.255$', '7C', 'GSM diagnostic'),
    (r'^0-0:29\.', '7B', 'G3-PLC setup'),
    (r'^0-0:25\.10\.0\.255$', '8', 'NTP, optional'),
    (r'^0-0:21\.0\.', '6c', 'display list, optional'),
    (r'^0-1:20\.0\.0\.255$', '6c', 'local port setup, CIP optional'),
    (r'^0-[1-4]:24\.', '9', 'M-Bus master/client and values'),
    (r'^0-[1-4]:96\.', '9', 'M-Bus device data'),
    (r'^0-[1-4]:10\.0\.(106|110|111)\.255$', '9', 'M-Bus script table'),
    (r'^0-[1-4]:15\.0\.(1|6|7)\.255$', '9', 'M-Bus scheduler'),
    (r'^0-[1-4]:13\.', '9', 'M-Bus calendar'),
    (r'^0-[1-4]:11\.', '9', 'M-Bus special days'),
    (r'^0-[1-4]:44\.1\.0\.255$', '9', 'M-Bus function control'),
    (r'^0-0:24\.6\.', '9', 'M-Bus master port'),
    (r'^0-1:24\.6\.', '9', 'M-Bus master port'),
    (r'^1-0:99\.97\.', '5', 'power failure event log'),
    (r'^0-[0-9]:25\.9\.', '6c', 'push setup'),
    (r'^0-0:96\.13\.', '6c', 'consumer message, optional'),
    (r'^1-0:(1|2|3|4|9|10)\.7\.0\.255$', '6a', 'instantaneous power'),
    (r'^1-0:(1|2|3|4|9|10)\.8\.', '6a', 'energy register'),
    (r'^1-0:0\.8\.2\.255$', '6a', 'meter constant'),
    (r'^1-0:12\.', '6b', 'power quality threshold'),
    (r'^1-0:(3[2-9]|5[2-9]|7[2-9])\.', '6b', 'power quality measurement'),
    (r'^1-0:(1|9|10|15|16)\.24\.0\.255$', '6a', 'average demand'),
    (r'^1-0:(31|51|71)\.4\.0\.255$', '6c', 'fuse supervision; OBIS conflict to resolve'),
]

MEDIA = [
    (r'^0-0:25\.2\.0\.255$', 'Ethernet,G3'),
    (r'^0-0:29\.', 'G3'),
    (r'^0-0:25\.(3|4|6)\.0\.255$', 'GPRS'),
    (r'^0-0:2\.0\.0\.255$', 'GPRS'),
    (r'^0-0:2\.2\.0\.255$', 'GPRS'),
]



DOMAIN_RULES = [
    (r'^0-0:43\.', 'security', 'AARQ/HLS -> nd_dlms_security -> nd_crypto_port(infra/security/crypto, ESAM) -> access check; keys in EEPROM', 'infra+protocol+target'),
    (r'^0-0:(40|41)\.', 'association', 'nd_dlms_link(HDLC/IP) -> APDU -> nd_dlms_assoc -> nd_cosem port; SAP table in EEPROM', 'protocol+target'),
    (r'^0-0:42\.|^0-0:96\.1\.', 'product', 'manufacturing/parameter store -> target product config -> nd_idis_system -> object value', 'target+protocol'),
    (r'^0-0:1\.0\.0\.255$', 'common.clock', 'RTC(infra/board) -> domain/common/calendar -> target clock port -> target cosem clock -> dlms GET/SET; EEPROM backup', 'infra+domain+application+protocol+target'),
    (r'^1-0:0\.9\.11\.255$', 'common.clock', 'parameter store -> target clock config -> target cosem clock time-shift limit', 'target+protocol'),
    (r'^0-0:96\.14\.0\.255$', 'tariff', 'domain/tariff -> target tariff_lookup -> target cosem active tariff', 'domain+application+protocol+target'),
    (r'^0-0:(13|11|14)\.', 'tariff', 'EEPROM parameter_store -> domain/tariff -> target tariff_lookup -> application/metering -> target cosem calendar/activation -> dlms', 'domain+application+protocol+target'),
    (r'^0-0:10\.0\.(100|1)\.255$', 'tariff', 'target cosem script table -> script engine -> target tariff/register methods -> domain/tariff', 'domain+protocol+target'),
    (r'^0-0:15\.0\.0\.255$', 'freeze', 'domain/freeze scheduler -> target billing trigger -> target profile billing buffer', 'domain+protocol+target'),
    (r'^1-0:[1-8]\.8\.', 'metering', 'pulse/计量前端(infra/board) -> domain/metering energy ledger -> target metering_energy_adapter -> domain/metering energy API -> target cosem register -> dlms GET; persistence target/persistence/legacy_energy_*', 'infra+domain+application+protocol+target'),
    (r'^1-0:(15|16)\.8\.', 'metering', 'domain/metering energy ledger -> target metering_energy_adapter -> domain/metering energy API -> target cosem register', 'domain+application+protocol+target'),
    (r'^1-0:[1-4]\.4\.0\.255$', 'metering.demand', 'metering pulses -> domain/metering/demand -> target demand adapter -> domain API -> target cosem demand register', 'domain+application+protocol+target'),
    (r'^1-0:[1-4]\.6\.', 'metering.demand+freeze', 'domain/metering/demand -> billing/freeze boundary -> target demand adapter -> target cosem extended register; SPI flash', 'domain+protocol+target'),
    (r'^1-0:(1|2|3|4|9|10)\.7\.|^1-0:(31|32|51|52|71|72|90)\.7\.|^1-0:(31|32|51|52|71|72)\.24\.', 'metering.measurement', 'RN8209/ADC -> infra/board -> target metering adapter -> domain/metering instantaneous API -> target cosem register', 'infra+domain+application+protocol+target'),
    (r'^0-0:98\.1\.0\.255$', 'freeze', 'domain/freeze planner -> target freeze adapter -> target profile billing buffer(SPI flash) -> target cosem profile -> dlms selective read', 'domain+protocol+target'),
    (r'^1-0:99\.1\.0\.255$|^1-0:99\.2\.0\.255$', 'metering+freeze', 'scheduler -> domain/metering energy ledger -> target profile capture -> target profile(SPI flash) -> target cosem profile', 'domain+protocol+target'),
    (r'^1-0:99\.14\.0\.255$', 'events+freeze', 'power-quality detectors -> domain/events -> target profile capture -> target profile(SPI flash) -> target cosem PQ profile', 'domain+protocol+target'),
    (r'^0-0:99\.98\.|^1-0:99\.97\.|^0-0:96\.10\.', 'events/freeze', 'domain/events or profile scheduler -> target log/profile store(SPI flash) -> target cosem IC7 -> dlms selective read', 'domain+protocol+target'),
    (r'^0-0:97\.98\.|^0-0:16\.1\.|^0-0:96\.11\.|^0-0:96\.15\.|^0-0:97\.97\.|^1-0:(32|52|72)\.(32|33|34|36|37|38)\.|^0-0:96\.7\.', 'events', '计量/板级输入(infra/board) -> domain/events detectors -> target event adapter -> target cosem alarm/event/counter -> dlms GET or PUSH; EEPROM/SPI flash', 'infra+domain+protocol+target'),
    (r'^0-0:96\.3\.10\.255$|^0-0:15\.0\.1\.255$|^0-0:13\.0\.1\.255$|^0-0:11\.0\.1\.255$|^0-0:10\.0\.106\.255$|^0-0:17\.0\.0\.255$|^0-0:10\.0\.103\.255$|^0-[0-9]+:96\.3\.10\.255$|^0-0:13\.0\.2\.255$|^0-0:11\.0\.2\.255$', 'load_control', 'target cosem script/schedule -> domain/load_control relay API -> target relay_adapter -> infra/board relay; feedback board -> load_control', 'infra+domain+application+protocol+target'),
    (r'^0-[0-9]:25\.9\.|^0-0:10\.0\.108\.255$|^0-[0-9]:15\.0\.4\.255$', 'protocol.push', 'trigger (scheduler/alarm/monitor) -> target cosem push -> protocol.dlms data-notification -> security -> nd_ip_port -> target net', 'protocol+services+target'),
    (r'^0-0:44\.1\.0\.255$|^0-0:15\.0\.7\.255$|^0-0:10\.0\.111\.255$', 'protocol.function', 'function control status -> profile capture gate -> domain/metering or freeze; script table -> set_function_status', 'domain+protocol+target'),
    (r'^0-0:25\.10\.', 'common.clock+services.time', 'NTP client(target/services.time) -> domain/common/calendar -> clock object', 'domain+services+target'),
    (r'^0-0:44\.0\.0\.255$|^0-0:15\.0\.2\.255$|^0-0:10\.0\.107\.255$|^1-0:0\.2\.', 'upgrade', 'dlms image transfer -> target flash -> signature verify -> bootloader; application/upgrade staging', 'application+protocol+target'),
    (r'^0-0:25\.0\.0\.255$|^0-0:25\.1\.0\.255$|^0-0:25\.7\.0\.255$|^0-0:25\.2\.0\.255$|^0-0:2\.1\.0\.255$|^0-0:25\.3\.0\.255$|^0-0:25\.4\.0\.255$|^0-0:25\.6\.0\.255$|^0-0:2\.0\.0\.255$|^0-0:2\.2\.0\.255$|^0-0:29\.', 'network', 'target cosem comm object -> nd_ip_port -> target lwip/G3/PPP -> Ethernet/G3 radio/GPRS', 'protocol+services+infra+target'),
    (r'^0-0:20\.0\.0\.255$|^0-0:22\.0\.0\.255$|^0-1:20\.0\.0\.255$', 'protocol.link', 'infra/board UART/optical -> nd_hdlc_link -> protocol.dlms', 'infra+protocol+target'),
    (r'^0-0:21\.0\.', 'display', 'target cosem display list -> domain/display -> target LCD; optional', 'domain+protocol+target'),
    (r'^0-0:96\.13\.', 'display/protocol', 'target cosem consumer message -> display or CIP; optional', 'domain+protocol+target'),
    (r'^0-[1-4]:24\.|^0-[1-4]:96\.|^0-[1-4]:10\.0\.(106|110|111)\.255$|^0-[1-4]:15\.0\.(1|6|7)\.255$|^0-[1-4]:13\.|^0-[1-4]:11\.|^0-[1-4]:44\.1\.0\.255$|^0-0:24\.6\.|^0-1:24\.6\.', 'm-bus', 'target cosem M-Bus objects -> target M-Bus master/wireless driver -> M-Bus port', 'infra+protocol+target'),
    (r'^1-0:(9|10)\.8\.', 'metering', 'domain/metering energy ledger -> target metering_energy_adapter -> domain/metering energy API -> target cosem register', 'domain+application+protocol+target'),
    (r'^1-0:(9|10)\.6\.', 'metering.demand+freeze', 'domain/metering/demand -> freeze boundary -> target demand adapter -> target cosem extended register', 'domain+protocol+target'),
    (r'^1-0:(1|2|3|4|9|10)\.5\.', 'metering.demand', 'domain/metering/demand -> target demand adapter -> target cosem register', 'domain+protocol+target'),
    (r'^1-0:(9|10)\.4\.0\.255$', 'metering.demand', 'domain/metering/demand -> target demand adapter -> target cosem demand register', 'domain+protocol+target'),
    (r'^1-0:12\.', 'events', 'power-quality threshold -> domain/events -> target event adapter -> target cosem register', 'domain+protocol+target'),
    (r'^1-0:13\.|^1-0:14\.', 'metering.measurement', 'RN8209/ADC -> infra/board -> target metering adapter -> port -> target cosem register', 'infra+domain+application+protocol+target'),
    (r'^1-0:(15|16)\.7\.|^1-0:(15|16)\.24\.', 'metering.measurement', 'RN8209/ADC -> infra/board -> target metering adapter -> port -> target cosem register', 'infra+domain+application+protocol+target'),
    (r'^1-0:(31|51|71)\.4\.0\.255$', 'metering+load_control', 'fuse supervision; class 21 monitor and class 5 sliding average share the OBIS; resolve before implementation', 'domain+protocol+target'),
    (r'^1-0:96\.10\.1\.255$', 'events/freeze', 'PQ profile status alias used by the object model; IP profile uses 0-0:96.10.4.255', 'domain+protocol+target'),
    (r'^1-[12]:0\.2\.', 'upgrade', 'firmware version/signature -> target flash/bootloader', 'application+protocol+target'),
    (r'^0-1:22\.0\.0\.255$', 'protocol.link', 'M-Bus local HDLC port -> nd_hdlc_link -> dlms; deferred', 'infra+protocol+target'),
    (r'^0-0:2\.2\.1\.255$', 'network', 'IP profile 8.3 typo; use auto connect 0-0:2.1.0.255 -> nd_ip_port', 'protocol+services+target'),
    (r'^1-0:0\.8\.2\.255$', 'metering', 'meter constant -> domain/metering energy ledger', 'domain+application+target'),
    (r'^1-0:0\.9\.(1|2)\.255$', 'common.clock', 'RTC -> domain/common/calendar -> local time/date objects', 'infra+domain+protocol+target'),
    (r'^1-0:(1|9|10|15|16)\.24\.0\.255$', 'metering.demand', 'domain/metering/demand -> target demand adapter -> target cosem register', 'domain+protocol+target'),
]


def domain_for(obis, member, kind):
    for pat, domain, lifecycle, layers in DOMAIN_RULES:
        if re.match(pat, obis):
            break
    else:
        domain, lifecycle, layers = 'unmapped', 'unmapped', ''
    if kind == 'method':
        if member == 'reset' and domain.startswith('metering'):
            lifecycle += '; reset -> metering_clear -> target persistence'
        elif member == 'reset':
            lifecycle += '; reset -> profile/log reset'
        elif member == 'execute':
            lifecycle += '; execute -> script engine -> target object method -> owning domain'
        elif member in ('remote_disconnect', 'remote_reconnect'):
            lifecycle += '; action -> domain/load_control command'
        elif member == 'set_function_status':
            lifecycle += '; action -> profile capture gate'
        elif member.startswith('image_'):
            lifecycle += '; action -> upgrade image pipeline'
        elif member in ('push',):
            lifecycle += '; push -> data-notification'
        elif member == 'synchronize_time':
            lifecycle += '; action -> clock set'
        elif member == 'connect':
            lifecycle += '; action -> nd_ip_port connect'
        elif member.startswith(('add_', 'remove_', 'delete_', 'get_')):
            lifecycle += '; config write -> EEPROM'
    return domain, lifecycle, layers


def access(cell):
    head = re.split(r'\s{2,}', cell.strip(), 1)[0]
    head = head.replace('(', '').replace(')', '')
    found = [t for t in ('Get', 'Set', 'Action')
             if re.search(r'\b' + t + r'\b', head, re.I)]
    return ','.join(found) if found else '--'


METHOD_NAMES = {
    'reset', 'next_period', 'execute', 'push', 'capture', 'slave_install',
    'slave_deinstall', 'remote_disconnect', 'remote_reconnect',
    'connect_logical_device', 'security_activate', 'global_key_transfer',
    'key_agreement', 'generate_key_pair', 'generate_certificate_request',
    'import_certificate', 'export_certificate', 'remove_certificate',
    'reply_to_HLS_authentication', 'change_HLS_secret', 'add_object',
    'remove_object', 'add_user', 'remove_user', 'image_transfer_initiate',
    'image_block_transfer', 'image_verify', 'image_activate',
    'add_mc_IP_address', 'delete_mc_IP_address', 'get_nbof_mc_IP_addresses',
    'add_IPv6_address', 'remove_IPv6_address', 'synchronize_time',
    'add_authentication_key', 'delete_authentication_key',
    'delete _authentication_key', 'set_function_status',
    'activate_passive_calendar', 'add_mask', 'delete_mask',
    'mac_get_neighbour_table_entry',
}


def is_method(row):
    name = row['attr_name'].strip()
    access_text = access(row['public']) + access(row['preest']) + access(row['mgmt'])
    return name in METHOD_NAMES or 'Action' in access_text


def phase_for(obis, ic):
    for pat, phase, owner in PHASE:
        if re.match(pat, obis):
            return phase, owner
    fallback = {
        '15': ('3', 'association'), '17': ('3', 'SAP assignment'),
        '64': ('4', 'security setup'), '7': ('5', 'profile/log'),
        '40': ('6c', 'push'), '41': ('7', 'TCP-UDP'), '42': ('7', 'IPv4'),
        '48': ('7', 'IPv6'), '43': ('7A/7B', 'MAC setup'),
        '44': ('7C', 'PPP'), '45': ('7C', 'GPRS'), '47': ('7C', 'GSM'),
        '27': ('7C', 'modem'), '28': ('7C', 'auto answer'),
        '29': ('7', 'auto connect'), '90': ('7B', 'G3'), '91': ('7B', 'G3'),
        '92': ('7B', 'G3'), '72': ('9', 'M-Bus client'),
        '74': ('9', 'M-Bus port'), '122': ('6c', 'function control'),
        '70': ('6c', 'disconnect/relay'), '71': ('6c', 'limiter'),
        '19': ('3', 'local port'), '23': ('3', 'HDLC optical'),
        '18': ('8', 'image transfer'), '100': ('8', 'NTP'),
        '1': ('6b', 'data object'), '3': ('6a', 'register'),
        '4': ('6a', 'extended register'), '5': ('6a', 'demand register'),
        '6': ('6a', 'register activation'), '8': ('6a', 'clock'),
        '9': ('6a', 'script table'), '11': ('6a', 'special days'),
        '20': ('6a', 'activity calendar'), '21': ('6b', 'register monitor'),
        '22': ('6a', 'single action schedule'),
    }
    return fallback.get(ic, ('TBD', 'unmapped'))


def media_for(obis):
    for pat, media in MEDIA:
        if re.match(pat, obis):
            return media
    return ''


def main():
    objects = list(csv.DictReader(open(OBIS, encoding='utf-8')))
    model = list(csv.DictReader(open(OM, encoding='utf-8')))
    by_obis = defaultdict(list)
    for r in model:
        if r['attr_index']:
            by_obis[r['obis']].append(r)
    template = {}
    for ob, rows in by_obis.items():
        ic = rows[0]['class']
        if ic not in template or len(rows) > len(template[ic][1]):
            template[ic] = (ob, rows)

    out = []
    for obj in objects:
        ob = obj['obis']
        rows = by_obis.get(ob)
        source = 'parsed'
        if not rows and obj['class_id'].split(',')[0] in template:
            tob, rows = template[obj['class_id'].split(',')[0]]
            source = f'class-template from {tob}'
        if not rows:
            out.append({'obis': ob, 'ic': obj['class_id'], 'version': obj['version'],
                        'object': obj['name'], 'kind': 'unresolved', 'id': '',
                        'member': 'class definition missing', 'type': '',
                        'public': '', 'preest': '', 'mgmt': '', 'default': '',
                        'phase': 'TBD', 'owner': 'Blue Book IC definition',
                        'domain': 'unmapped', 'lifecycle': 'unmapped', 'layers': '',
                        'device': '104/105', 'medium': media_for(ob),
                        'source': source, 'page': ''})
            continue
        for r in rows:
            kind = 'method' if is_method(r) else 'attribute'
            phase, owner = phase_for(ob, obj['class_id'].split(',')[0])
            flags = obj.get('flags', '')
            if '/' in flags:
                d104, d105 = flags.split('/', 1)
            elif flags:
                d104 = d105 = flags
            else:
                d104 = d105 = ''
            if d104 == 'NA' and d105:
                device = '105'
            elif d105 == 'NA' and d104:
                device = '104'
            elif re.search(r'(L2|L3|\.52\.|\.72\.|\.51\.|\.71\.)', ob + ' ' + r['attr_name']):
                device = '105'
            else:
                device = '104/105'
            domain, lifecycle, layers = domain_for(ob, r['attr_name'], kind)
            out.append({'obis': ob, 'ic': obj['class_id'], 'version': obj['version'],
                        'object': obj['name'], 'kind': kind,
                        'id': r['attr_index'], 'member': r['attr_name'],
                        'type': r['attr_type'].strip(), 'public': access(r['public']),
                        'preest': access(r['preest']), 'mgmt': access(r['mgmt']),
                        'default': r['attr_tag'].strip(), 'phase': phase,
                        'owner': owner, 'domain': domain, 'lifecycle': lifecycle,
                        'layers': layers, 'device': device, 'medium': media_for(ob),
                        'source': source, 'page': r['page']})

    # Blue Book 4.3.6 defines method 2 capture for every Profile Generic object.
    # The IDIS object model omits the row, but IDIS 7.6 requires the capture method.
    have = {(r['obis'], r['kind'], r['id']) for r in out}
    extra = []
    for r in out:
        if r['ic'] != '7':
            continue
        key = (r['obis'], 'method', '2')
        if key in have:
            continue
        have.add(key)
        extra.append(dict(r, kind='method', id='2', member='capture',
                          type='integer', public='--', preest='--', mgmt='--',
                          default='', phase=r['phase'],
                          owner='profile capture',
                          lifecycle=r['lifecycle'] + '; capture -> append profile entry',
                          source='Blue Book 4.3.6', note='optional in Blue Book, required by IDIS 7.6'))
    out.extend(extra)
    # Blue Book Profile Generic v0 has mandatory methods 3 and 4.
    for r in list(out):
        if r['ic'] != '7' or r['version'] != '0':
            continue
        for mid, name in (('3', 'get_buffer_by_range'), ('4', 'get_buffer_by_index')):
            key = (r['obis'], 'method', mid)
            if key in have:
                continue
            have.add(key)
            out.append(dict(r, kind='method', id=mid, member=name,
                            type='integer', public='--', preest='--', mgmt='--',
                            default='', owner='profile buffer access',
                            source='Blue Book 4.3.6',
                            note='mandatory in Profile Generic v0'))
    for r in out:
        r.setdefault('note', '')

    with open(OUT, 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=['obis', 'ic', 'version', 'object', 'kind',
                                          'id', 'member', 'type', 'public', 'preest',
                                          'mgmt', 'default', 'phase', 'owner', 'domain',
                                          'lifecycle', 'layers', 'device', 'medium',
                                          'source', 'page', 'note'])
        w.writeheader()
        w.writerows(out)

    with open(MD, 'w', encoding='utf-8') as f:
        f.write('# IDIS P3 OBIS 成员矩阵\n\n')
        f.write(f'共 {len(objects)} 个 OBIS，{len(out)} 条属性/方法行。\n\n')
        current = None
        for r in out:
            if r['obis'] != current:
                current = r['obis']
                f.write(f"\n## {r['obis']} IC {r['ic']} {r['object']}\n\n")
                f.write('| 类型 | id | 名称 | 类型 | Public | Preest | Mgmt | 阶段 | domain | 生命周期与组件层次 | 设备 | medium | 备注 |\n')
                f.write('|---|---|---|---|---|---|---|---|---|---|---|---|\n')
            f.write(f"| {r['kind']} | {r['id']} | {r['member']} | {r['type']} | {r['public']} | {r['preest']} | {r['mgmt']} | {r['phase']} | {r['domain']} | {r['lifecycle']} | {r['device']} | {r['medium']} | {r['note']} |\n")

    by_domain = defaultdict(lambda: {'obis': {}, 'members': 0, 'lifecycle': set()})
    for r in out:
        d = by_domain[r['domain']]
        d['obis'][r['obis']] = r['object']
        d['members'] += 1
        d['lifecycle'].add(r['lifecycle'])

    status = {}
    status_path = 'docs/idis/obis_status.csv'
    with open(status_path, newline='', encoding='utf-8') as sf:
        for row in csv.DictReader(sf):
            status[row['obis']] = (row['status'], row.get('note', ''))
    labels = {
        'done': '✅ 已实现',
        'partial': '◐ 部分',
        'gap': '○ 缺口',
        'blocked': '⛔ 阻塞',
        'invalid': '✕ 无效',
    }

    def status_of(obis):
        return status.get(obis, ('gap', '未标注'))

    def cell(text):
        return text.replace('|', '\\|')

    totals = Counter(status_of(o)[0] for d in by_domain.values() for o in d['obis'])
    with open('docs/idis/obis_by_domain.md', 'w', encoding='utf-8') as f:
        f.write('# IDIS P3 OBIS 按 domain 归属\n\n')
        f.write(f'共 {len(out)} 条成员行，{len(by_domain)} 个 domain，'
                f'{sum(len(d["obis"]) for d in by_domain.values())} 个 OBIS。\n\n')
        f.write('状态：✅ 已实现 / ◐ 部分 / ○ 缺口 / ⛔ 阻塞 / ✕ 无效。\n\n')
        f.write('- ✅ 已实现：对象已在 COSEM 端口注册，核心属性/方法可用。\n'
                '- ◐ 部分：对象已注册，但存在占位或未建模的属性/方法/实例。\n'
                '- ○ 缺口：对象未实现，补类或实例即可，不依赖新硬件。\n'
                '- ⛔ 阻塞：依赖硬件、EN 13757/网络协议栈、安全材料或协议栈。\n'
                '- ✕ 无效：OBIS 字段非法（如 256），不是真实对象。\n\n'
                '状态由 `docs/idis/obis_status.csv` 维护，按当前代码映射核对。\n\n')
        f.write(f"合计：✅ {totals['done']}，◐ {totals['partial']}，"
                f"○ {totals['gap']}，⛔ {totals['blocked']}，"
                f"✕ {totals['invalid']}。\n\n")
        f.write('| domain | OBIS | ✅ | ◐ | ○ | ⛔ | ✕ |\n')
        f.write('|---|---:|---:|---:|---:|---:|---:|\n')
        for d in sorted(by_domain, key=lambda x: -by_domain[x]['members']):
            v = by_domain[d]
            c = Counter(status_of(o)[0] for o in v['obis'])
            f.write(f"| {d} | {len(v['obis'])} | {c['done']} | {c['partial']} | "
                    f"{c['gap']} | {c['blocked']} | {c['invalid']} |\n")
        f.write('\n')
        for d in sorted(by_domain, key=lambda x: -by_domain[x]['members']):
            v = by_domain[d]
            c = Counter(status_of(o)[0] for o in v['obis'])
            f.write(f"## {d}\n\n成员行 {v['members']}，OBIS {len(v['obis'])} 个。\n\n")
            f.write(f"状态：✅ {c['done']} / ◐ {c['partial']} / ○ {c['gap']} / "
                    f"⛔ {c['blocked']} / ✕ {c['invalid']}。\n\n")
            f.write('| OBIS | 对象 | 状态 | 说明 |\n|---|---|---|---|\n')
            for o, n in sorted(v['obis'].items()):
                st, note = status_of(o)
                f.write(f"| `{o}` | {cell(n)} | {labels[st]} | {cell(note)} |\n")
            f.write('\n生命周期链路样例。\n\n')
            for l in sorted(v['lifecycle'])[:5]:
                f.write(f'- {l}\n')
            f.write('\n')
        missing = [o for d in by_domain.values() for o in d['obis'] if o not in status]
        if missing:
            print(f'warning: {len(missing)} OBIS without status')

    print(f'{len(objects)} OBIS, {len(out)} member rows -> {OUT}')
    print('kinds', Counter(r['kind'] for r in out))
    print('phases', Counter(r['phase'] for r in out))


if __name__ == '__main__':
    main()
