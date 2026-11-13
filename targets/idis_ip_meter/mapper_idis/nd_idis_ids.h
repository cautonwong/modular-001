#ifndef ND_IDIS_IDS_H
#define ND_IDIS_IDS_H

#include "components/protocol/dlms/include/nd_axdr.h"

/* IDIS Pack3 OBIS constants. Hand-curated from docs/idis/all_obis.csv. */
static const nd_obis_t ND_OBIS_CLOCK = {0, 0, 1, 0, 0, 255};
static const nd_obis_t ND_OBIS_VOLTAGE_L1 = {1, 0, 32, 7, 0, 255};
static const nd_obis_t ND_OBIS_VOLTAGE_L2 = {1, 0, 52, 7, 0, 255};
static const nd_obis_t ND_OBIS_VOLTAGE_L3 = {1, 0, 72, 7, 0, 255};
static const nd_obis_t ND_OBIS_CURRENT_L1 = {1, 0, 31, 7, 0, 255};
static const nd_obis_t ND_OBIS_CURRENT_L2 = {1, 0, 51, 7, 0, 255};
static const nd_obis_t ND_OBIS_CURRENT_L3 = {1, 0, 71, 7, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_POWER_IMPORT = {1, 0, 1, 7, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_POWER_EXPORT = {1, 0, 2, 7, 0, 255};
static const nd_obis_t ND_OBIS_REACTIVE_POWER_IMPORT = {1, 0, 3, 7, 0, 255};
static const nd_obis_t ND_OBIS_REACTIVE_POWER_EXPORT = {1, 0, 4, 7, 0, 255};
static const nd_obis_t ND_OBIS_APPARENT_POWER_IMPORT = {1, 0, 9, 7, 0, 255};
static const nd_obis_t ND_OBIS_APPARENT_POWER_EXPORT = {1, 0, 10, 7, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_POWER_COMBINED = {1, 0, 15, 7, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_POWER_NET = {1, 0, 16, 7, 0, 255};
static const nd_obis_t ND_OBIS_CURRENT_SUM = {1, 0, 90, 7, 0, 255};
static const nd_obis_t ND_OBIS_POWER_FACTOR_MIN = {1, 0, 13, 3, 0, 255};
static const nd_obis_t ND_OBIS_POWER_FACTOR_AVG = {1, 0, 13, 5, 0, 255};
static const nd_obis_t ND_OBIS_MEASUREMENT_PERIOD_3 = {1, 0, 0, 8, 2, 255};
static const nd_obis_t ND_OBIS_LDN = {0, 0, 42, 0, 0, 255};
static const nd_obis_t ND_OBIS_LOCAL_TIME = {1, 0, 0, 9, 1, 255};
static const nd_obis_t ND_OBIS_LOCAL_DATE = {1, 0, 0, 9, 2, 255};
static const nd_obis_t ND_OBIS_CONSUMER_TEXT = {0, 0, 96, 13, 0, 255};
static const nd_obis_t ND_OBIS_CONSUMER_CODE = {0, 0, 96, 13, 1, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_PQ_ALT = {1, 0, 96, 10, 1, 255};
static const nd_obis_t ND_OBIS_DEVICE_IDS[6] = {
    {0, 0, 96, 1, 0, 255}, {0, 0, 96, 1, 1, 255},
    {0, 0, 96, 1, 2, 255}, {0, 0, 96, 1, 3, 255},
    {0, 0, 96, 1, 4, 255}, {0, 0, 96, 1, 5, 255},
};
static const nd_obis_t ND_OBIS_MBUS_DEVICE_IDS[4][2] = {
    {{0, 1, 96, 1, 0, 255}, {0, 1, 96, 1, 1, 255}},
    {{0, 2, 96, 1, 0, 255}, {0, 2, 96, 1, 1, 255}},
    {{0, 3, 96, 1, 0, 255}, {0, 3, 96, 1, 1, 255}},
    {{0, 4, 96, 1, 0, 255}, {0, 4, 96, 1, 1, 255}},
};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_MBUS[4] = {
    {0, 1, 96, 10, 3, 255}, {0, 2, 96, 10, 3, 255},
    {0, 3, 96, 10, 3, 255}, {0, 4, 96, 10, 3, 255},
};
static const nd_obis_t ND_OBIS_MBUS_EVENT_OBJECTS[4] = {
    {0, 1, 96, 11, 4, 255}, {0, 2, 96, 11, 4, 255},
    {0, 3, 96, 11, 4, 255}, {0, 4, 96, 11, 4, 255},
};
static const nd_obis_t ND_OBIS_POWER_FACTOR = {1, 0, 13, 7, 0, 255};
static const nd_obis_t ND_OBIS_FREQUENCY = {1, 0, 14, 7, 0, 255};
static const nd_obis_t ND_OBIS_BILLING_PROFILE = {0, 0, 98, 1, 0, 255};
static const nd_obis_t ND_OBIS_LOAD_PROFILE_1 = {1, 0, 99, 1, 0, 255};
static const nd_obis_t ND_OBIS_LOAD_PROFILE_2 = {1, 0, 99, 2, 0, 255};
static const nd_obis_t ND_OBIS_PQ_PROFILE = {1, 0, 99, 14, 0, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_STANDARD = {0, 0, 96, 11, 0, 255};
static const nd_obis_t ND_OBIS_ALARM_REGISTER_1 = {0, 0, 97, 98, 0, 255};
static const nd_obis_t ND_OBIS_EVENT_COUNTER_1 = {0, 0, 96, 15, 0, 255};
static const nd_obis_t ND_OBIS_EVENT_COUNTERS[12] = {
    {0, 0, 96, 15, 0, 255},  {0, 0, 96, 15, 1, 255},
    {0, 0, 96, 15, 2, 255},  {0, 0, 96, 15, 3, 255},
    {0, 0, 96, 15, 4, 255},  {0, 0, 96, 15, 5, 255},
    {0, 0, 96, 15, 6, 255},  {0, 0, 96, 15, 7, 255},
    {0, 0, 96, 15, 8, 255},  {0, 0, 96, 15, 9, 255},
    {0, 0, 96, 15, 10, 255}, {0, 0, 96, 15, 11, 255},
};
static const nd_obis_t ND_OBIS_ALARM_REGISTER_2 = {0, 0, 97, 98, 1, 255};
static const nd_obis_t ND_OBIS_ALARM_FILTER_1 = {0, 0, 97, 98, 10, 255};
static const nd_obis_t ND_OBIS_ALARM_FILTER_2 = {0, 0, 97, 98, 11, 255};
static const nd_obis_t ND_OBIS_ALARM_DESCRIPTOR_1 = {0, 0, 97, 98, 20, 255};
static const nd_obis_t ND_OBIS_ALARM_DESCRIPTOR_2 = {0, 0, 97, 98, 21, 255};
static const nd_obis_t ND_OBIS_ERROR_REGISTER = {0, 0, 97, 97, 0, 255};
static const nd_obis_t ND_OBIS_POWER_FAILURE_COUNT = {0, 0, 96, 7, 21, 255};
static const nd_obis_t ND_OBIS_POWER_FAILURE_LONG_COUNT = {0, 0, 96, 7, 9, 255};
static const nd_obis_t ND_OBIS_POWER_FAILURE_THRESHOLD = {0, 0, 96, 7, 20, 255};
static const nd_obis_t ND_OBIS_PQ_SAG_COUNT[3] = {
    {1, 0, 32, 32, 0, 255}, {1, 0, 52, 32, 0, 255}, {1, 0, 72, 32, 0, 255}};
static const nd_obis_t ND_OBIS_PQ_SAG_DURATION[3] = {
    {1, 0, 32, 33, 0, 255}, {1, 0, 52, 33, 0, 255}, {1, 0, 72, 33, 0, 255}};
static const nd_obis_t ND_OBIS_PQ_SAG_MAGNITUDE[3] = {
    {1, 0, 32, 34, 0, 255}, {1, 0, 52, 34, 0, 255}, {1, 0, 72, 34, 0, 255}};
static const nd_obis_t ND_OBIS_PQ_SWELL_COUNT[3] = {
    {1, 0, 32, 36, 0, 255}, {1, 0, 52, 36, 0, 255}, {1, 0, 72, 36, 0, 255}};
static const nd_obis_t ND_OBIS_PQ_SWELL_DURATION[3] = {
    {1, 0, 32, 37, 0, 255}, {1, 0, 52, 37, 0, 255}, {1, 0, 72, 37, 0, 255}};
static const nd_obis_t ND_OBIS_PQ_SWELL_MAGNITUDE[3] = {
    {1, 0, 32, 38, 0, 255}, {1, 0, 52, 38, 0, 255}, {1, 0, 72, 38, 0, 255}};
static const nd_obis_t ND_OBIS_STANDARD_EVENT_LOG = {0, 0, 99, 98, 0, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_1 = {0, 0, 96, 10, 1, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_2 = {0, 0, 96, 10, 2, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_PQ = {0, 0, 96, 10, 4, 255};
static const nd_obis_t ND_OBIS_ACTIVE_ENERGY_IMPORT = {1, 0, 1, 8, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_ENERGY_EXPORT = {1, 0, 2, 8, 0, 255};
static const nd_obis_t ND_OBIS_REACTIVE_ENERGY_IMPORT = {1, 0, 3, 8, 0, 255};
static const nd_obis_t ND_OBIS_REACTIVE_ENERGY_EXPORT = {1, 0, 4, 8, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_ENERGY_COMBINED = {1, 0, 15, 8, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVE_ENERGY_NET = {1, 0, 16, 8, 0, 255};
static const nd_obis_t ND_OBIS_DEMAND_ACTIVE_IMPORT = {1, 0, 1, 4, 0, 255};
static const nd_obis_t ND_OBIS_DEMAND_ACTIVE_EXPORT = {1, 0, 2, 4, 0, 255};
static const nd_obis_t ND_OBIS_DEMAND_REACTIVE_IMPORT = {1, 0, 3, 4, 0, 255};
static const nd_obis_t ND_OBIS_DEMAND_REACTIVE_EXPORT = {1, 0, 4, 4, 0, 255};
static const nd_obis_t ND_OBIS_DISCONNECT_CONTROL = {0, 0, 96, 3, 10, 255};
static const nd_obis_t ND_OBIS_ACTIVITY_CALENDAR = {0, 0, 13, 0, 0, 255};
static const nd_obis_t ND_OBIS_ACTIVITY_CALENDARS[4] = {
    {0, 0, 13, 0, 0, 255}, {0, 0, 13, 0, 1, 255},
    {0, 0, 13, 0, 2, 255}, {0, 1, 13, 0, 1, 255},
};
static const nd_obis_t ND_OBIS_SPECIAL_DAYS = {0, 0, 11, 0, 0, 255};
static const nd_obis_t ND_OBIS_SPECIAL_DAYS_TABLES[4] = {
    {0, 0, 11, 0, 0, 255}, {0, 0, 11, 0, 1, 255},
    {0, 0, 11, 0, 2, 255}, {0, 1, 11, 0, 1, 255},
};
static const nd_obis_t ND_OBIS_AVERAGE_VOLTAGE_L1 = {1, 0, 32, 24, 0, 255};
static const nd_obis_t ND_OBIS_AVERAGE_VOLTAGE_L2 = {1, 0, 52, 24, 0, 255};
static const nd_obis_t ND_OBIS_AVERAGE_VOLTAGE_L3 = {1, 0, 72, 24, 0, 255};
static const nd_obis_t ND_OBIS_AVERAGE_CURRENT_L1 = {1, 0, 31, 24, 0, 255};
static const nd_obis_t ND_OBIS_AVERAGE_CURRENT_L2 = {1, 0, 51, 24, 0, 255};
static const nd_obis_t ND_OBIS_AVERAGE_CURRENT_L3 = {1, 0, 71, 24, 0, 255};

/* Remaining Pack3 profile objects. */
static const nd_obis_t ND_OBIS_FRAUD_LOG = {0, 0, 99, 98, 1, 255};
static const nd_obis_t ND_OBIS_DISCONNECTOR_LOG = {0, 0, 99, 98, 2, 255};
static const nd_obis_t ND_OBIS_MBUS_EVENT_LOG = {0, 0, 99, 98, 3, 255};
static const nd_obis_t ND_OBIS_PQ_LOG = {0, 0, 99, 98, 4, 255};
static const nd_obis_t ND_OBIS_COMM_LOG = {0, 0, 99, 98, 5, 255};
static const nd_obis_t ND_OBIS_COMM_DETAILS_LOG = {0, 0, 99, 98, 6, 255};
static const nd_obis_t ND_OBIS_POWER_FAILURE_LOG = {1, 0, 99, 97, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_LOAD_PROFILE_1 = {0, 1, 24, 3, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_LOAD_PROFILE_2 = {0, 2, 24, 3, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_LOAD_PROFILE_3 = {0, 3, 24, 3, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_LOAD_PROFILE_4 = {0, 4, 24, 3, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CONTROL_LOG_1 = {0, 1, 24, 5, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CONTROL_LOG_2 = {0, 2, 24, 5, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CONTROL_LOG_3 = {0, 3, 24, 5, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CONTROL_LOG_4 = {0, 4, 24, 5, 0, 255};
static const nd_obis_t ND_OBIS_DISPLAY_GENERAL = {0, 0, 21, 0, 1, 255};
static const nd_obis_t ND_OBIS_DISPLAY_ALTERNATE = {0, 0, 21, 0, 2, 255};

/* Event objects captured by the event log profiles. */
static const nd_obis_t ND_OBIS_EVENT_OBJECT_FRAUD = {0, 0, 96, 11, 1, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_DISCONNECTOR = {0, 0, 96, 11, 2, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_MBUS = {0, 0, 96, 11, 3, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_PQ = {0, 0, 96, 11, 4, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_COMM = {0, 0, 96, 11, 5, 255};
static const nd_obis_t ND_OBIS_EVENT_OBJECT_COMM_DETAILS = {0, 0, 96, 11, 6, 255};
static const nd_obis_t ND_OBIS_POWER_FAILURE_DURATION = {0, 0, 96, 7, 19, 255};

/* M-Bus profile status registers, one per channel. */
static const nd_obis_t ND_OBIS_PROFILE_STATUS_MBUS_1 = {0, 1, 96, 10, 3, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_MBUS_2 = {0, 2, 96, 10, 3, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_MBUS_3 = {0, 3, 96, 10, 3, 255};
static const nd_obis_t ND_OBIS_PROFILE_STATUS_MBUS_4 = {0, 4, 96, 10, 3, 255};

/* M-Bus value registers captured by the M-Bus load profiles. */
#define ND_OBIS_CONST(name, a_, b_, c_, d_, e_, f_) \
    static const nd_obis_t name = {a_, b_, c_, d_, e_, f_}
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_1, 0, 1, 24, 2, 1, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_2, 0, 1, 24, 2, 2, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_3, 0, 1, 24, 2, 3, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_4, 0, 1, 24, 2, 4, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_1, 0, 2, 24, 2, 1, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_2, 0, 2, 24, 2, 2, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_3, 0, 2, 24, 2, 3, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_4, 0, 2, 24, 2, 4, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_1, 0, 3, 24, 2, 1, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_2, 0, 3, 24, 2, 2, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_3, 0, 3, 24, 2, 3, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_4, 0, 3, 24, 2, 4, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_1, 0, 4, 24, 2, 1, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_2, 0, 4, 24, 2, 2, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_3, 0, 4, 24, 2, 3, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_4, 0, 4, 24, 2, 4, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_5, 0, 1, 24, 2, 5, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_6, 0, 1, 24, 2, 6, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_7, 0, 1, 24, 2, 7, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_1_8, 0, 1, 24, 2, 8, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_5, 0, 2, 24, 2, 5, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_6, 0, 2, 24, 2, 6, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_7, 0, 2, 24, 2, 7, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_2_8, 0, 2, 24, 2, 8, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_5, 0, 3, 24, 2, 5, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_6, 0, 3, 24, 2, 6, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_7, 0, 3, 24, 2, 7, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_3_8, 0, 3, 24, 2, 8, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_5, 0, 4, 24, 2, 5, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_6, 0, 4, 24, 2, 6, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_7, 0, 4, 24, 2, 7, 255);
ND_OBIS_CONST(ND_OBIS_MBUS_VALUE_4_8, 0, 4, 24, 2, 8, 255);
#undef ND_OBIS_CONST

/* M-Bus client (IC72) channels and master port setup (IC74) objects. */
static const nd_obis_t ND_OBIS_MBUS_CLIENT_1 = {0, 1, 24, 1, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CLIENT_2 = {0, 2, 24, 1, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CLIENT_3 = {0, 3, 24, 1, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_CLIENT_4 = {0, 4, 24, 1, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_PORT_SETUP_1 = {0, 0, 24, 6, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_PORT_SETUP_2 = {0, 1, 24, 6, 0, 255};

/* Script tables (IC9) and their targets. */
static const nd_obis_t ND_OBIS_SCRIPT_TARIFF = {0, 0, 10, 0, 100, 255};
static const nd_obis_t ND_OBIS_SCRIPT_MDI = {0, 0, 10, 0, 1, 255};
static const nd_obis_t ND_OBIS_SCRIPT_DISCONNECT = {0, 0, 10, 0, 106, 255};
static const nd_obis_t ND_OBIS_SCRIPT_LOAD_MGMT = {0, 0, 10, 0, 103, 255};
static const nd_obis_t ND_OBIS_SCRIPT_IMAGE = {0, 0, 10, 0, 107, 255};
static const nd_obis_t ND_OBIS_SCRIPT_PUSH = {0, 0, 10, 0, 108, 255};
static const nd_obis_t ND_OBIS_SCRIPT_FUNCTION_CONTROL = {0, 0, 10, 0, 111, 255};
static const nd_obis_t ND_OBIS_SCRIPT_MBUS_DISCONNECT = {0, 1, 10, 0, 106, 255};
static const nd_obis_t ND_OBIS_SCRIPT_MBUS_FUNCTION_CONTROL = {0, 1, 10, 0, 111, 255};
static const nd_obis_t ND_OBIS_SCRIPT_MBUS_PROFILE_CONTROL = {0, 11, 10, 0, 110, 255};

/* Single action schedules (IC22). */
static const nd_obis_t ND_OBIS_SINGLE_ACTION_BILLING = {0, 0, 15, 0, 0, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_DISCONNECT = {0, 0, 15, 0, 1, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_IMAGE = {0, 0, 15, 0, 2, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_FUNCTION = {0, 0, 15, 0, 7, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_MBUS_DISCONNECT = {0, 1, 15, 0, 1, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_MBUS_FUNCTION = {0, 1, 15, 0, 7, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_PUSH_1 = {0, 1, 15, 0, 4, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_PUSH_2 = {0, 2, 15, 0, 4, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_PUSH_3 = {0, 3, 15, 0, 4, 255};
static const nd_obis_t ND_OBIS_SINGLE_ACTION_PUSH_4 = {0, 4, 15, 0, 4, 255};
/* Parameter registers: clock shift limit and power quality thresholds. */
static const nd_obis_t ND_OBIS_CLOCK_SHIFT_LIMIT = {1, 0, 0, 9, 11, 255};
static const nd_obis_t ND_OBIS_PQ_SAG_THRESHOLD = {1, 0, 12, 31, 0, 255};
static const nd_obis_t ND_OBIS_PQ_SAG_TIME = {1, 0, 12, 43, 0, 255};
static const nd_obis_t ND_OBIS_PQ_SWELL_THRESHOLD = {1, 0, 12, 35, 0, 255};
static const nd_obis_t ND_OBIS_PQ_SWELL_TIME = {1, 0, 12, 44, 0, 255};
static const nd_obis_t ND_OBIS_PQ_CUT_THRESHOLD = {1, 0, 12, 39, 0, 255};
static const nd_obis_t ND_OBIS_PQ_CUT_TIME = {1, 0, 12, 45, 0, 255};

/* Unimplemented targets referenced by the script tables. */
static const nd_obis_t ND_OBIS_MBUS_DISCONNECT_1 = {0, 1, 24, 4, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_DISCONNECT_2 = {0, 2, 24, 4, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_DISCONNECT_3 = {0, 3, 24, 4, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_DISCONNECT_4 = {0, 4, 24, 4, 0, 255};
static const nd_obis_t ND_OBIS_MBUS_FUNCTION_CONTROL = {0, 1, 44, 1, 0, 255};
static const nd_obis_t ND_OBIS_FUNCTION_CONTROL = {0, 0, 44, 1, 0, 255};
static const nd_obis_t ND_OBIS_TARIFF_ACTIVE_MASK_1 = {0, 0, 14, 0, 1, 255};
static const nd_obis_t ND_OBIS_TARIFF_ACTIVE_MASK_2 = {0, 0, 14, 0, 2, 255};
static const nd_obis_t ND_OBIS_ACTIVE_TARIFF = {0, 0, 96, 14, 0, 255};
static const nd_obis_t ND_OBIS_LOAD_CONTROL_1 = {0, 1, 96, 3, 10, 255};
static const nd_obis_t ND_OBIS_LOAD_CONTROL_2 = {0, 2, 96, 3, 10, 255};
static const nd_obis_t ND_OBIS_LIMITER = {0, 0, 17, 0, 0, 255};
static const nd_obis_t ND_OBIS_IMAGE_TRANSFER = {0, 0, 44, 0, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_0 = {0, 0, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_1 = {0, 1, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_2 = {0, 2, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_3 = {0, 3, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_4 = {0, 4, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_5 = {0, 5, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_6 = {0, 6, 25, 9, 0, 255};
static const nd_obis_t ND_OBIS_PUSH_7 = {0, 7, 25, 9, 0, 255};

static inline int nd_obis_eq(const nd_obis_t *left, const nd_obis_t *right)
{
    return left->a == right->a && left->b == right->b && left->c == right->c
        && left->d == right->d && left->e == right->e && left->f == right->f;
}


/* Association, SAP and security setup objects. */
static const nd_obis_t ND_OBIS_ASSOCIATION = {0, 0, 40, 0, 0, 255};
static const nd_obis_t ND_OBIS_SAP_ASSIGNMENT = {0, 0, 41, 0, 0, 255};
static const nd_obis_t ND_OBIS_SECURITY_SETUP = {0, 0, 43, 0, 0, 255};
static const nd_obis_t ND_OBIS_SECURITY_SETUP_CONSUMER = {0, 0, 43, 0, 1, 255};
static const nd_obis_t ND_OBIS_SECURITY_FC_UNICAST = {0, 0, 43, 1, 0, 255};
static const nd_obis_t ND_OBIS_SECURITY_FC_BROADCAST = {0, 0, 43, 1, 1, 255};

static const nd_obis_t ND_OBIS_FIRMWARE_IDS[3] = {
    {1, 0, 0, 2, 0, 255}, {1, 1, 0, 2, 0, 255}, {1, 2, 0, 2, 0, 255}};
static const nd_obis_t ND_OBIS_FIRMWARE_SIGNATURES[3] = {
    {1, 0, 0, 2, 8, 255}, {1, 1, 0, 2, 8, 255}, {1, 2, 0, 2, 8, 255}};

#endif
