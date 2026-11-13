#include <stdbool.h>
#include "application/adapters/dl698/fr_report_frames.h"

#include "fr_698_env.h"
#include "parameter_port.h"
#include "report_list.h"
#include "meter_base_view.h"
#include "domain/common/status.h"
#include "domain/report/report_policy.h"
#include "domain/report/report_retry.h"
#include "report_encoder.h"

int active_report_3011_pro(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg, uint8_t eventtype)
{
    uint8_t data_buf[500] = {0};
    nd_report_encoder_t encoder;
    uint32_t rcsd_list[(REQ_RCSD_MAXNUM + 1)];
    uint32_t oad;
    uint16_t data_len = 0;
    uint16_t apdu_len = 0;
    uint8_t *esam_apdu_p;
    uint8_t esam_tail[17] = {0};
    uint8_t data_tag = 1;
    uint8_t end_tag[2] = {0, 0};
    struct rsd_info_t rsd_info;
    int ret;

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }

    if (false == report_channel_check(instance, FR_698_CHL_PLC))
    {
        return 0;
    }

    if (false == active_report_check_3011(instance, pipeline, mesg,
                                          nd_fr698_apdu_cache(instance) + 500,
                                          eventtype))
    {
        return 0;
    }

    if (nd_report_encoder_init(&encoder, data_buf, sizeof(data_buf)) != 0)
    {
        return 0;
    }
    if (nd_report_encoder_begin_3011(&encoder, nd_mb_report_piid(instance),
                                     eventtype) != 0)
    {
        return 0;
    }
    esam_apdu_p = data_buf + encoder.apdu_start;

    oad = 0x30110200;
    get_capture_list(instance, oad, rcsd_list, 3);
    if (rcsd_list[0] > REQ_RCSD_MAXNUM)
    {
        return -1;
    }
    if (nd_report_encoder_3011_rcsd(&encoder, &rcsd_list[1],
                                    (uint8_t)rcsd_list[0]) != 0)
    {
        return 0;
    }
    if (nd_report_encoder_append(&encoder, &data_tag, 1) != 0)
    {
        return 0;
    }

    memset((uint8_t *)&rsd_info, 0, sizeof(rsd_info));
    rsd_info.select = 9;
    rsd_info.last_no = 1;
    rsd_info.start_mark = 0;
    rsd_info.end_mark = 0xffffffffU;
    rsd_info.mode = 0;

    ret = record_data_package(instance, oad, nd_fr698_apdu_cache(instance) + 500,
                              &rsd_info, rcsd_list, 300, NULL);
    if (ret < 0)
    {
        return 0;
    }
    data_len = ret;
    if (nd_report_encoder_append(&encoder,
                                 nd_fr698_apdu_cache(instance) + 500,
                                 data_len) != 0)
    {
        return 0;
    }
    if (nd_report_encoder_append(&encoder, end_tag, sizeof(end_tag)) != 0)
    {
        return 0;
    }

    apdu_len = encoder.offset - encoder.apdu_start;
    if (nd_report_encoder_finish_3011(&encoder) != 0)
    {
        return 0;
    }

    if (eventtype)
    {
        int esam_abnormal = (nd_mb_esam_chip_abnormal(instance)
                             == ND_EVENT_SET);

        if (!esam_abnormal)
        {
            get_upreport_esam_rn_mac(instance, esam_apdu_p, apdu_len,
                                     esam_tail);
        }
        if (nd_report_encoder_rn_mac_tail(&encoder,
                                          nd_mb_report_random_data(instance),
                                          esam_abnormal ? NULL : esam_tail,
                                          esam_abnormal) != 0)
        {
            return 0;
        }
    }

    ndv_memcopy(nd_fr698_apdu_cache(instance), data_buf, encoder.offset);
    return (int)encoder.offset;
}

//获取模块通道 3320上报列表中主动上报 oad
//函数返回值为主动上报oad个数，    传入buf空间需保证300字节 
uint8_t active_report_3320list_get(struct instance_t* instance, struct pipeline_t*, nd_dl698_msg_t*, uint8_t* buf)
{
    nd_report_list_t list;
    uint8_t tmp_buf[241];
    uint8_t i, count = 0, rptmode;
    uint8_t *p = buf;

    if (dl698_parameter_read(DL698_PARAM_REPORT_3320_LIST_PLC, tmp_buf,
                             sizeof(tmp_buf)) != 0)
    {
        return 0;
    }
    if (nd_report_list_parse(&list, tmp_buf, sizeof(tmp_buf)) != 0)
    {
        return 0;
    }

    for (i = 0; i < list.count; i++)
    {
        rptmode = read_event_report_flag_mode(instance, list.oad[i], 1);
        if ((rptmode == FOLLOW_RPT) || (list.oad[i] == 0x30110200))
        {
            continue;
        }

        *p++ = TYPE_OAD;
        mem_swap_cpy(p, &tmp_buf[1 + (size_t)i * 4], 4);
        p += 4;
        count++;
    }
    return count;
}
//2015跟随上报状态字  检查是否需要上报，及获取状态字
//mode: 0 主动上报， 1跟随上报
uint32_t report_2015_get(struct instance_t* instance, struct pipeline_t*, nd_dl698_msg_t*, uint16_t chn, enum report_mode_t mode)
{
    uint32_t value;

    uint8_t rptmode = 0;

    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);

    if (rptmode != mode)//上报方式为跟随上报，才能跟随上报
    {
        return 0;
    }

    value = nd_mb_report_2015_state(instance)[chn%4];  //FR_698_CHL_PLC

    return value;
}
// 常规的3320和2015 主动上报组帧(apdu)
// get_data:0 只判断是否需要上报。  其他:获取一次上报内容
int active_report_normal_pro(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg, uint8_t get_data, nd_report_retry_t* retry)
{
    uint8_t* apdu, num, count = 0;
    uint32_t value = 0;
    uint16_t len_apdu = 0;
    int include_3320 = 0, include_2015 = 0;
    uint8_t data_buf[500] = {0};
    uint8_t esam_tail[17] = {0};
    uint8_t* tmp_buff = nd_fr698_apdu_cache(instance) + 500;

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }

    if (false == report_channel_check(instance, FR_698_CHL_PLC))
    {
        return 0;
    }


    num = 0;

    if (retry->cmdtype & ND_REPORT_CMD_3320)
    {
        count = active_report_3320list_get(instance, pipeline, mesg, &tmp_buff[0]);
    }
    if (retry->cmdtype & ND_REPORT_CMD_2015)
    {
        value = report_2015_get(instance, pipeline, mesg, FR_698_CHL_PLC, ACTIVE_RPT);
    }

    nd_report_retry_normal_prepare(retry, get_data, count != 0, value != 0,
                                   &include_3320, &include_2015);

    if (!include_3320 && !include_2015)
    {
        return 0;
    }

    if (include_3320)
    {
        num++;
    }
    if (include_2015)
    {
        num++;
    }

    if (num == 0)
    {
        return 0;
    }
    nd_report_encoder_t encoder;

    if (nd_report_encoder_init(&encoder, data_buf, sizeof(data_buf)) != 0)
    {
        return 0;
    }
    if (nd_report_encoder_begin_normal(&encoder,
                                       nd_mb_report_piid(instance), num) != 0)
    {
        return 0;
    }
    if (include_3320
        && (nd_report_encoder_3320(&encoder, &tmp_buff[0], count) != 0))
    {
        return 0;
    }
    if (include_2015
        && (nd_report_encoder_2015(&encoder, value) != 0))
    {
        return 0;
    }
    if (nd_report_encoder_end_normal(&encoder) != 0)
    {
        return 0;
    }

    apdu = data_buf + encoder.apdu_start;
    len_apdu = (uint16_t)(encoder.offset - encoder.apdu_start);

    if (nd_mb_esam_chip_abnormal(instance) == ND_EVENT_SET)
    {
        if (nd_report_encoder_rn_mac_tail(&encoder,
                                          nd_mb_report_random_data(instance),
                                          NULL, 1) != 0)
        {
            return 0;
        }
    }
    else
    {
        get_upreport_esam_rn_mac(instance, apdu, len_apdu, esam_tail);
        if (nd_report_encoder_rn_mac_tail(&encoder,
                                          nd_mb_report_random_data(instance),
                                          esam_tail, 0) != 0)
        {
            return 0;
        }
    }

    ndv_memcopy(nd_fr698_apdu_cache(instance), data_buf, encoder.offset);

    return (int)encoder.offset;
}

