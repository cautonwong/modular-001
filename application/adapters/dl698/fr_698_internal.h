#ifndef ND_APPLICATION_DL698_FR_698_INTERNAL_H
#define ND_APPLICATION_DL698_FR_698_INTERNAL_H

#include <stdint.h>
#include "fr_698.h"

int __get_apdu_data_len(struct context_698_t* ctx);
int fr_segments_handle(struct instance_t* instance, struct pipeline_t* pipeline,
                       nd_dl698_msg_t* mesg);
int handle_scrambler(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int __fr_segment_tx_handle(struct instance_t* instance,
                           struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int __fr_send_handle(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_check_service_and_choice(struct instance_t* instance,
                                struct pipeline_t* pipeline,
                                nd_dl698_msg_t* mesg);
int fr698_inital(struct instance_t* instance, struct pipeline_t* pipeline);

int fr_ctx_clear(struct instance_t* instance, struct pipeline_t* pipeline,
                 nd_dl698_msg_t* mesg);

int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline,
                        nd_dl698_msg_t* mesg);
int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline,
                      nd_dl698_msg_t* mesg);
int fr_handle_disconnect(struct instance_t* instance,
                         struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_handle_get_request(struct instance_t* instance,
                          struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_handle_set_action_request(struct instance_t* instance,
                                 struct pipeline_t* pipeline,
                                 nd_dl698_msg_t* mesg);
int fr_follow_report_optional(struct instance_t* instance,
                              struct pipeline_t* pipeline,
                              nd_dl698_msg_t* mesg);
int get_CONNECT_Request_len(struct instance_t* instance,
                            struct pipeline_t* pipeline,
                            nd_dl698_msg_t* mesg);
int get_GET_Request_len(struct instance_t* instance,
                        struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int get_SET_and_ACTION_Request_len(struct instance_t* instance,
                                   struct pipeline_t* pipeline,
                                   nd_dl698_msg_t* mesg);
int support_broadcast_addr_cmd_check(uint8_t cmd, uint32_t oad);

int __matcher_698(void* pctx, uint8_t* b);
int __pack_apdu_byte(void* pctx, uint8_t* byte);
int fr_pack_698_head(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tx_head(struct instance_t* instance,
                        struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_pack_698_apdu(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tail(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_rx_698_end(struct instance_t* instance, struct pipeline_t* pipeline,
                  nd_dl698_msg_t* mesg);
int fr_timetag_check(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_check(struct instance_t* instance, struct pipeline_t* pipeline,
                      nd_dl698_msg_t* mesg);
int fr_addr_handle(struct instance_t* instance, struct pipeline_t* pipeline,
                   nd_dl698_msg_t* mesg);
int fr_apdu_effective_check(struct instance_t* instance,
                            struct pipeline_t* pipeline,
                            nd_dl698_msg_t* mesg);
int fr_timeTag_optional(struct instance_t* instance,
                        struct pipeline_t* pipeline,
                        nd_dl698_msg_t* mesg);

#endif
