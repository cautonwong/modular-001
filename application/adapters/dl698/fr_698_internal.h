#ifndef ND_APPLICATION_DL698_FR_698_INTERNAL_H
#define ND_APPLICATION_DL698_FR_698_INTERNAL_H

#include <stdint.h>
#include "fr_698.h"

int __get_apdu_data_len(struct context_698_t* ctx);
int fr_segments_handle(void * instance, void * pipeline,
                       nd_dl698_msg_t* mesg);
int handle_scrambler(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int __fr_segment_tx_handle(void * instance,
                           void * pipeline, nd_dl698_msg_t* mesg);
int __fr_send_handle(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_check_service_and_choice(void * instance,
                                void * pipeline,
                                nd_dl698_msg_t* mesg);
int fr698_inital(void * instance, void * pipeline);

int fr_ctx_clear(void * instance, void * pipeline,
                 nd_dl698_msg_t* mesg);

int fr_security_request(void * instance, void * pipeline,
                        nd_dl698_msg_t* mesg);
int fr_handle_connect(void * instance, void * pipeline,
                      nd_dl698_msg_t* mesg);
int fr_handle_disconnect(void * instance,
                         void * pipeline, nd_dl698_msg_t* mesg);
int fr_handle_get_request(void * instance,
                          void * pipeline, nd_dl698_msg_t* mesg);
int fr_handle_set_action_request(void * instance,
                                 void * pipeline,
                                 nd_dl698_msg_t* mesg);
int fr_follow_report_optional(void * instance,
                              void * pipeline,
                              nd_dl698_msg_t* mesg);
int get_CONNECT_Request_len(void * instance,
                            void * pipeline,
                            nd_dl698_msg_t* mesg);
int get_GET_Request_len(void * instance,
                        void * pipeline, nd_dl698_msg_t* mesg);
int get_SET_and_ACTION_Request_len(void * instance,
                                   void * pipeline,
                                   nd_dl698_msg_t* mesg);
int support_broadcast_addr_cmd_check(uint8_t cmd, uint32_t oad);

int __matcher_698(void* pctx, uint8_t* b);
int __pack_apdu_byte(void* pctx, uint8_t* byte);
int fr_handle_action_request(void * instance, void * pipeline,
                             nd_dl698_msg_t* mesg);
uint8_t fr_698_current_channel_num_get(void * instance,
                                       void * pipeline);
uint16_t get_crc16(uint16_t cs);
int fr_pack_698_head(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tx_head(void * instance,
                        void * pipeline, nd_dl698_msg_t* mesg);
int fr_pack_698_apdu(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tail(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_rx_698_end(void * instance, void * pipeline,
                  nd_dl698_msg_t* mesg);
int fr_timetag_check(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_check(void * instance, void * pipeline,
                      nd_dl698_msg_t* mesg);
int fr_addr_handle(void * instance, void * pipeline,
                   nd_dl698_msg_t* mesg);
int fr_apdu_effective_check(void * instance,
                            void * pipeline,
                            nd_dl698_msg_t* mesg);
int fr_timeTag_optional(void * instance,
                        void * pipeline,
                        nd_dl698_msg_t* mesg);

#endif
