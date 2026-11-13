#ifndef ND_IDIS_OBJECTS_H
#define ND_IDIS_OBJECTS_H

#include "targets/idis_ip_meter/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register.h"
#include "domain/load_control/disconnect.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_demand.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_calendar.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_extended_register.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/nd_idis_average.h"
#include "targets/idis_ip_meter/cosem/nd_idis_pq.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_disconnect.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_activation.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_function_control.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_push.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_single_action.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_monitor.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_limiter.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_image_transfer.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_setup.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_association.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event.h"
#include "targets/idis_ip_meter/cosem/nd_idis_scheduler.h"
#include "domain/metering/energy_set.h"
#include "targets/idis_ip_meter/cosem/nd_idis_mbus.h"
#include "domain/events/power_failure.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"

/*
 * Single target-owned container for COSEM class state. The port passes the
 * class slice to each handler; this is not an object lookup table.
 */
typedef struct {
    nd_idis_clock_state_t clock;
    nd_idis_register_state_t reg;
    nd_idis_profiles_t profiles;
    nd_idis_event_state_t event;
    nd_energy_set_t energy;
    nd_idis_demand_state_t demand[ND_IDIS_DEMAND_MAX];
    nd_idis_average_power_t average_power[2];
    nd_idis_demand_acc_t demand_acc[ND_IDIS_DEMAND_MAX];
    nd_disconnect_snapshot_t disconnect;
    nd_disconnect_snapshot_t load_control_1;
    nd_disconnect_snapshot_t load_control_2;
    nd_disconnect_snapshot_t mbus_disconnect[4];
    nd_register_activation_t activation;
    nd_function_control_t functions;
    nd_idis_push_t push;
    nd_single_action_t single_action;
    nd_power_failure_state_t power_failure;
    nd_idis_monitors_t monitors;
    nd_limiter_state_t limiter;
    nd_image_state_t image;
    nd_idis_setup_t setup;
    nd_idis_association_t association;
    uint32_t security_fc_rx[2];
    uint8_t firmware_id[3][16];
    uint8_t firmware_id_len[3];
    uint8_t firmware_sig[3][32];
    uint8_t firmware_sig_len[3];
    uint8_t ldn_len;
    uint8_t ldn[16];
    uint8_t device_id_len[6];
    uint8_t device_id[6][16];
    uint8_t consumer_text_len;
    uint8_t consumer_text[64];
    uint8_t consumer_code_len;
    uint8_t consumer_code[64];
    nd_idis_mbus_t mbus;
    nd_idis_calendar_state_t calendar;
    nd_idis_calendar_state_t calendar_extra[3];
    nd_idis_extended_state_t max_demand[ND_IDIS_MAX_DEMAND];
    nd_idis_average_state_t average;
    nd_pq_state_t pq;
    nd_script_state_t scripts;
    nd_idis_scheduler_state_t scheduler;
    uint8_t access_enforced;
    uint8_t remote_client;
} nd_idis_objects_t;

#endif
