#ifndef ND_APPLICATION_APPS_APP_ENTRY_H
#define ND_APPLICATION_APPS_APP_ENTRY_H

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

#endif
