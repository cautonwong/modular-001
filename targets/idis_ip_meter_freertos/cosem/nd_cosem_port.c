#include "components/protocol/dlms/include/nd_cosem_port.h"

#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_demand.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_calendar.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_extended_register.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_disconnect.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_data.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_profile_generic.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_register.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_register_activation.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_function_control.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_push.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_client.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_value.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_mbus_port_setup.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_single_action.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_register_monitor.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_limiter.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_image_transfer.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_setup.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_association.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

enum {
    ND_ACCESS_GET = 1,
    ND_ACCESS_SET = 2,
    ND_ACCESS_ACTION = 4,
};

static int local_only_script_table(const nd_obis_t *ln)
{
    return nd_obis_eq(ln, &ND_OBIS_SCRIPT_LOAD_MGMT)
        || nd_obis_eq(ln, &ND_OBIS_SCRIPT_DISCONNECT)
        || nd_obis_eq(ln, &ND_OBIS_SCRIPT_MBUS_DISCONNECT)
        || nd_obis_eq(ln, &ND_OBIS_SCRIPT_FUNCTION_CONTROL)
        || nd_obis_eq(ln, &ND_OBIS_SCRIPT_MBUS_FUNCTION_CONTROL);
}

static int access_allowed(const nd_idis_objects_t *objects, uint8_t sap,
                          uint8_t access)
{
    if (objects == NULL || !objects->access_enforced) {
        return 1;
    }
    switch (sap) {
    case 1:
        return 1;
    case 16:
        return access == ND_ACCESS_GET;
    case 102:
        return access == ND_ACCESS_SET || access == ND_ACCESS_ACTION;
    default:
        return 0;
    }
}

static nd_idis_calendar_state_t *calendar_state_for(
    nd_idis_objects_t *objects, const nd_obis_t *ln)
{
    for (uint8_t index = 0; index < 4; index++) {
        if (nd_obis_eq(ln, &ND_OBIS_ACTIVITY_CALENDARS[index])
            || nd_obis_eq(ln, &ND_OBIS_SPECIAL_DAYS_TABLES[index])) {
            return index == 0 ? &objects->calendar
                              : &objects->calendar_extra[index - 1];
        }
    }
    return NULL;
}

int nd_cosem_port_get(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      const nd_axdr_reader_t *selector,
                      nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;

    if (objects == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (!access_allowed(objects, client_sap, ND_ACCESS_GET)) {
        return ND_ERR_ACCESS_DENIED;
    }

    switch (class_id) {
    case ND_IC_CLOCK:
        return nd_ic_clock_get(&objects->clock, client_sap, ln, attribute_id,
                               selector, out);
    case ND_IC_REGISTER:
        return nd_ic_register_get(&objects->reg, client_sap, ln, attribute_id,
                                  selector, out);
    case ND_IC_PROFILE_GENERIC:
        return nd_ic_profile_generic_get(&objects->profiles, client_sap, ln,
                                         attribute_id, selector, out);
    case ND_IC_DEMAND_REGISTER:
        return nd_ic_demand_get(objects, client_sap, ln, attribute_id,
                                selector, out);
    case ND_IC_DISCONNECT_CONTROL:
        return nd_ic_disconnect_get(objects, client_sap, ln, attribute_id,
                                    selector, out);
    case ND_IC_ACTIVITY_CALENDAR:
    case ND_IC_SPECIAL_DAYS: {
        nd_idis_calendar_state_t *state = calendar_state_for(objects, ln);
        if (state == NULL) {
            return ND_ERR_OBJECT_UNDEFINED;
        }
        if (class_id == ND_IC_ACTIVITY_CALENDAR) {
            return nd_ic_calendar_get(state, client_sap, ln, attribute_id,
                                      selector, out);
        }
        return nd_ic_special_days_get(state, client_sap, ln, attribute_id,
                                      selector, out);
    }
    case ND_IC_EXTENDED_REGISTER:
        if (nd_ic_mbus_value_matches(ln)) {
            return nd_ic_mbus_value_get(&objects->mbus, client_sap, ln,
                                        attribute_id, selector, out);
        }
        if (ln != NULL
            && (nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_MIN)
                || nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_AVG))) {
            return nd_ic_register_get(&objects->reg, client_sap, ln,
                                      attribute_id, selector, out);
        }
        if (ln != NULL && ln->d == 5) {
            return nd_ic_demand_last_get(objects, client_sap, ln, attribute_id,
                                         selector, out);
        }
        return nd_ic_extended_get(objects->max_demand, client_sap, ln,
                                  attribute_id, selector, out);
    case ND_IC_DATA:
        if (nd_ic_pq_stat_matches(ln)) {
            return nd_ic_register_get(&objects->reg, client_sap, ln,
                                      attribute_id, selector, out);
        }
        return nd_ic_data_get(objects, client_sap, ln, attribute_id, selector,
                              out);
    case ND_IC_SCRIPT_TABLE:
        return nd_ic_script_get(&objects->scripts, client_sap, ln,
                                attribute_id, selector, out);
    case ND_IC_REGISTER_ACTIVATION:
        return nd_ic_register_activation_get(&objects->activation, client_sap,
                                             ln, attribute_id, selector, out);
    case ND_IC_FUNCTION_CONTROL:
        return nd_ic_function_control_get(&objects->functions, client_sap, ln,
                                          attribute_id, selector, out);
    case ND_IC_PUSH_SETUP:
        return nd_ic_push_get(objects, client_sap, ln, attribute_id, selector,
                              out);
    case ND_IC_MBUS_CLIENT:
        return nd_ic_mbus_client_get(&objects->mbus, client_sap, ln,
                                     attribute_id, selector, out);
    case ND_IC_MBUS_PORT_SETUP:
        return nd_ic_mbus_port_setup_get(&objects->mbus, client_sap, ln,
                                         attribute_id, selector, out);
    case ND_IC_SINGLE_ACTION:
        return nd_ic_single_action_get(objects, client_sap, ln, attribute_id,
                                       selector, out);
    case ND_IC_REGISTER_MONITOR:
        return nd_ic_register_monitor_get(&objects->monitors, client_sap, ln,
                                          attribute_id, selector, out);
    case ND_IC_LIMITER:
        return nd_ic_limiter_get(&objects->limiter, client_sap, ln,
                                 attribute_id, selector, out);
    case ND_IC_IMAGE_TRANSFER:
        return nd_ic_image_get(&objects->image, client_sap, ln, attribute_id,
                               selector, out);
    case ND_IC_ASSOCIATION_LN:
    case ND_IC_SAP_ASSIGNMENT:
    case ND_IC_SECURITY_SETUP:
        return nd_ic_association_get(&objects->association, client_sap, ln,
                                     attribute_id, selector, out);
    case ND_IC_IEC_LOCAL_PORT:
    case ND_IC_IEC_HDLC_SETUP:
    case ND_IC_MODEM_CONFIG:
    case ND_IC_AUTO_ANSWER:
    case ND_IC_AUTO_CONNECT:
    case ND_IC_TCP_UDP_SETUP:
    case ND_IC_IPV4_SETUP:
    case ND_IC_MAC_SETUP:
    case ND_IC_PPP_SETUP:
    case ND_IC_GPRS_SETUP:
    case ND_IC_GSM_DIAGNOSTIC:
    case ND_IC_IPV6_SETUP:
    case ND_IC_G3_MAC_COUNTERS:
    case ND_IC_G3_MAC_SETUP:
    case ND_IC_G3_ADP_SETUP:
    case ND_IC_NTP_SETUP:
        return nd_ic_setup_get(&objects->setup, client_sap, ln, attribute_id,
                               selector, out);
    default:
        return ND_ERR_OBJECT_UNDEFINED;
    }
}

int nd_cosem_port_set(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      nd_axdr_reader_t *in)
{
    nd_idis_objects_t *objects = ctx;
    if (objects == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (!access_allowed(objects, client_sap, ND_ACCESS_SET)) {
        return ND_ERR_ACCESS_DENIED;
    }
    if (class_id == ND_IC_ACTIVITY_CALENDAR) {
        nd_idis_calendar_state_t *state = calendar_state_for(objects, ln);
        return state != NULL
                   ? nd_ic_calendar_set(state, client_sap, ln, attribute_id, in)
                   : ND_ERR_OBJECT_UNDEFINED;
    }
    if (class_id == ND_IC_CLOCK) {
        return nd_ic_clock_set(&objects->clock, client_sap, ln, attribute_id,
                               in);
    }
    if (class_id == ND_IC_REGISTER) {
        return nd_ic_register_set(&objects->reg, client_sap, ln, attribute_id,
                                  in);
    }
    if (class_id == ND_IC_DISCONNECT_CONTROL) {
        return nd_ic_disconnect_set(objects, client_sap, ln, attribute_id, in);
    }
    if (class_id == ND_IC_SCRIPT_TABLE) {
        return nd_ic_script_set(&objects->scripts, client_sap, ln,
                                attribute_id, in);
    }
    if (class_id == ND_IC_REGISTER_ACTIVATION) {
        return nd_ic_register_activation_set(&objects->activation, client_sap,
                                             ln, attribute_id, in);
    }
    if (class_id == ND_IC_EXTENDED_REGISTER) {
        return nd_ic_mbus_value_set(&objects->mbus, client_sap, ln,
                                    attribute_id, in);
    }
    if (class_id == ND_IC_MBUS_CLIENT) {
        return nd_ic_mbus_client_set(&objects->mbus, client_sap, ln,
                                     attribute_id, in);
    }
    if (class_id == ND_IC_PUSH_SETUP) {
        return nd_ic_push_set(objects, client_sap, ln, attribute_id, in);
    }
    if (class_id == ND_IC_SINGLE_ACTION) {
        return nd_ic_single_action_set(objects, client_sap, ln, attribute_id,
                                       in);
    }
    if (class_id == ND_IC_DATA) {
        return nd_ic_data_set(objects, client_sap, ln, attribute_id, in);
    }
    if (class_id == ND_IC_REGISTER_MONITOR) {
        return nd_ic_register_monitor_set(&objects->monitors, client_sap, ln,
                                          attribute_id, in);
    }
    if (class_id == ND_IC_LIMITER) {
        return nd_ic_limiter_set(&objects->limiter, client_sap, ln,
                                 attribute_id, in);
    }
    if (class_id == ND_IC_ASSOCIATION_LN || class_id == ND_IC_SAP_ASSIGNMENT
        || class_id == ND_IC_SECURITY_SETUP) {
        return nd_ic_association_set(&objects->association, client_sap, ln,
                                     attribute_id, in);
    }
    if (class_id == ND_IC_IEC_LOCAL_PORT || class_id == ND_IC_IEC_HDLC_SETUP
        || class_id == ND_IC_MODEM_CONFIG || class_id == ND_IC_AUTO_ANSWER
        || class_id == ND_IC_AUTO_CONNECT || class_id == ND_IC_TCP_UDP_SETUP
        || class_id == ND_IC_IPV4_SETUP || class_id == ND_IC_MAC_SETUP
        || class_id == ND_IC_PPP_SETUP || class_id == ND_IC_GPRS_SETUP
        || class_id == ND_IC_GSM_DIAGNOSTIC || class_id == ND_IC_IPV6_SETUP
        || class_id == ND_IC_G3_MAC_COUNTERS
        || class_id == ND_IC_G3_MAC_SETUP || class_id == ND_IC_G3_ADP_SETUP
        || class_id == ND_IC_NTP_SETUP) {
        return nd_ic_setup_set(&objects->setup, client_sap, ln, attribute_id,
                               in);
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_cosem_port_action(void *ctx, uint8_t client_sap, uint16_t class_id,
                         const nd_obis_t *ln, uint8_t method_id,
                         nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;

    if (objects == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (!access_allowed(objects, client_sap, ND_ACCESS_ACTION)) {
        return ND_ERR_ACCESS_DENIED;
    }
    if (class_id == ND_IC_PROFILE_GENERIC) {
        int result = nd_ic_profile_generic_action(
            &objects->profiles, client_sap, ln, method_id, in, out);
        if (result == ND_OK && method_id == 2
            && nd_obis_eq(ln, &ND_OBIS_BILLING_PROFILE)) {
            objects->event.counters[1]++;
        }
        return result;
    }
    if (class_id == ND_IC_DEMAND_REGISTER) {
        return nd_ic_demand_action(objects, client_sap, ln, method_id,
                                   in, out);
    }
    if (class_id == ND_IC_EXTENDED_REGISTER
        && (nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_MIN)
            || nd_obis_eq(ln, &ND_OBIS_POWER_FACTOR_AVG))) {
        return nd_ic_register_action(&objects->reg, client_sap, ln, method_id,
                                     in, out);
    }
    if (class_id == ND_IC_EXTENDED_REGISTER && ln != NULL && ln->d == 6) {
        return nd_ic_extended_action(objects->max_demand, client_sap, ln,
                                     method_id, in, out);
    }
    if (class_id == ND_IC_EXTENDED_REGISTER && nd_ic_mbus_value_matches(ln)) {
        return nd_ic_mbus_value_action(&objects->mbus, client_sap, ln,
                                       method_id, in, out);
    }
    if (class_id == ND_IC_EXTENDED_REGISTER && ln != NULL && ln->d == 5) {
        return nd_ic_demand_last_action(objects, client_sap, ln, method_id,
                                        in, out);
    }
    if (class_id == ND_IC_DISCONNECT_CONTROL) {
        return nd_ic_disconnect_action(objects, client_sap, ln, method_id, in,
                                       out);
    }
    if (class_id == ND_IC_ACTIVITY_CALENDAR) {
        nd_idis_calendar_state_t *state = calendar_state_for(objects, ln);
        return state != NULL
                   ? nd_ic_calendar_action(state, client_sap, ln, method_id, in,
                                           out)
                   : ND_ERR_OBJECT_UNDEFINED;
    }
    if (class_id == ND_IC_CLOCK) {
        return nd_ic_clock_action(&objects->clock, client_sap, ln, method_id,
                                  in, out);
    }
    if (class_id == ND_IC_IMAGE_TRANSFER) {
        return nd_ic_image_action(&objects->image, client_sap, ln, method_id,
                                  in, out);
    }
    if (class_id == ND_IC_SPECIAL_DAYS) {
        nd_idis_calendar_state_t *state = calendar_state_for(objects, ln);
        return state != NULL
                   ? nd_ic_special_days_action(state, client_sap, ln, method_id,
                                               in, out)
                   : ND_ERR_OBJECT_UNDEFINED;
    }
    if (class_id == ND_IC_SCRIPT_TABLE) {
        if (local_only_script_table(ln)
            && (objects->remote_client || client_sap == 102)) {
            return ND_ERR_ACCESS_DENIED;
        }
        return nd_ic_script_action(&objects->scripts, client_sap, ln,
                                   method_id, in, out);
    }
    if (class_id == ND_IC_REGISTER) {
        return nd_ic_register_action(&objects->reg, client_sap, ln, method_id,
                                     in, out);
    }
    if (class_id == ND_IC_FUNCTION_CONTROL) {
        return nd_ic_function_control_action(&objects->functions, client_sap,
                                             ln, method_id, in, out);
    }
    if (class_id == ND_IC_PUSH_SETUP) {
        return nd_ic_push_action(objects, client_sap, ln, method_id, in, out);
    }
    if (class_id == ND_IC_MBUS_CLIENT) {
        return nd_ic_mbus_client_action(&objects->mbus, client_sap, ln,
                                        method_id, in, out);
    }
    if (class_id == ND_IC_ASSOCIATION_LN || class_id == ND_IC_SAP_ASSIGNMENT
        || class_id == ND_IC_SECURITY_SETUP) {
        return nd_ic_association_action(&objects->association, client_sap, ln,
                                        method_id, in, out);
    }
    if (class_id == ND_IC_IEC_LOCAL_PORT || class_id == ND_IC_IEC_HDLC_SETUP
        || class_id == ND_IC_MODEM_CONFIG || class_id == ND_IC_AUTO_ANSWER
        || class_id == ND_IC_AUTO_CONNECT || class_id == ND_IC_TCP_UDP_SETUP
        || class_id == ND_IC_IPV4_SETUP || class_id == ND_IC_MAC_SETUP
        || class_id == ND_IC_PPP_SETUP || class_id == ND_IC_GPRS_SETUP
        || class_id == ND_IC_GSM_DIAGNOSTIC || class_id == ND_IC_IPV6_SETUP
        || class_id == ND_IC_G3_MAC_COUNTERS
        || class_id == ND_IC_G3_MAC_SETUP || class_id == ND_IC_G3_ADP_SETUP
        || class_id == ND_IC_NTP_SETUP) {
        return nd_ic_setup_action(&objects->setup, client_sap, ln, method_id,
                                  in, out);
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}
