#ifndef ND_TARGET_STATEGRID_METER_APP_ENTRY_H
#define ND_TARGET_STATEGRID_METER_APP_ENTRY_H

struct instance_t;
struct pipeline_t;
struct message_t;

int app_rx_server_698_entry(struct instance_t *instance,
                            struct pipeline_t *pipeline,
                            struct message_t *mesg);
int app_tx_server_698_entry(struct instance_t *instance,
                            struct pipeline_t *pipeline,
                            struct message_t *mesg);
int app_rx_server_645_entry(struct instance_t *instance,
                            struct pipeline_t *pipeline,
                            struct message_t *mesg);
int app_meter_base_entry(struct instance_t *instance,
                         struct pipeline_t *pipeline,
                         struct message_t *mesg);
int app_meter_lowpower_wakeup_entry(struct instance_t *instance,
                                    struct pipeline_t *pipeline,
                                    struct message_t *mesg);
int sync_data(struct instance_t *instance, struct pipeline_t *pipeline,
              struct message_t *mesg);
int PowerOff_Save_data(struct instance_t *instance, struct pipeline_t *pipeline,
                       struct message_t *mesg);
int baremetal_instance_entry(struct instance_t *instance);
int baremetal_instance_entry_main(struct instance_t *instance, int argc,
                                  char **argv);
extern struct pipeline_t pl_meter_base;
extern struct pipeline_t pl_rx_698;

#endif
