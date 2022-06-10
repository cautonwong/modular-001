#include "nidavellir.h"
#include "fr_698.h"

extern int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_get_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_handle_set_action_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

extern int fr_handle_disconnect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

extern int get_CONNECT_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int get_GET_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int get_SET_and_ACTION_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

extern int fr_follow_report_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

uint16_t get_crc16(uint16_t cs)
{
    cs ^= 0xffff;
    return cs;
}

int update_hcs_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->hcs = update_crc16(ctx->hcs, *byte);
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int update_fcs(void* pctx, uint8_t* byte)
{
    struct context_698_t* ctx = pctx;
    ctx->tx_len++;
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int __pack_apdu_byte(void* pctx, uint8_t* byte)
{
    struct pipeline_t* pipeline = pctx;
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    if (is_scrambler_set(ctx->ctrl)){
        *byte += 0x33;
    }
    append_byte(fifo, *byte, update_fcs, ctx);
    return 0;
}

int __matcher_698(void* pctx, uint8_t* b)
{
    uint8_t byte = *b;
    struct context_698_t* ctx = pctx;

    switch (ctx->step)
    {
    case 0://帧头68判断
        if (byte != 0x68){
            ctx->err_n++;
            return FRM_HEADER_ERR;
        }
        ctx->step = 1;
        return FRM_OK;
    case 1:
        ctx->hcs = update_crc16(0xffff, byte);
        ctx->fcs = update_crc16(0xffff, byte);
        ctx->min_len = byte;
        ctx->step = 2;
        return FRM_OK;
    case 2:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->min_len |= (byte << 8);
        ctx->min_len &= 0x3fff;
        //bit14：帧数据长度单位，0 表示帧数据长度单位为字节，1 表示帧数据长度单位为千字节；
        ctx->len_unit = (byte & 0x40) >> 6;
        if (ctx->min_len < 11){
            ctx->step = 0;
            ctx->err_n++;
            return FRM_LEN_ERR1;
        }
        //if ((len < 11) || (len + 2 > *bytes) || (0x16 != p[len + 1]))
        ctx->step = 3;
        return FRM_OK;
    case 3:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if ( (FC_CODE(byte) != 1) && (FC_CODE(byte) != 3)){
            ctx->step = 0;
            return -1;
        }
        ctx->ctrl = byte;//控制域
        ctx->step = 4;
        return FRM_OK;
    case 4:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->SA = byte;//SA
        ctx->index = 0;
        ctx->step = 6;
        if(SA_LOGIC_TYPE(ctx->SA)){
            ctx->step = 5; 
        }
        return FRM_OK;

    case 5: //扩展逻辑地址处理
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->LA = byte;
        ctx->index = 0;
        ctx->step = 6;
        return FRM_OK;

    case 6:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (ctx->index == SA_LEN(ctx->SA) + 1){
            ctx->CA = byte;
            ctx->step = 7;
        }else{
            ctx->addr[ctx->index++] = byte;
        }
        return FRM_OK;
    case 7:
        ctx->hcs = get_crc16(ctx->hcs);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != (ctx->hcs & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 8;
        return FRM_OK;
    case 8:
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != ((ctx->hcs >> 8) & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->index = 0;
        ctx->step = 9;
        return FRM_OK;
    case 9://用户链路数据
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (ctx->min_len > 2048)// 长度大于2048 暂时删除一个字节  为了台体能跑下去。  注意： 还需要超时删除报文！
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        
        if (ctx->index == ctx->min_len - 11 - SA_LEN(ctx->SA))//超过最大长度未测试？？
        {
            ctx->step = 10;
            return FRM_OK;
        }
        ctx->index++;
        return FRM_OK;
    case 10:
        ctx->fcs = get_crc16(ctx->fcs);
        if (byte != (ctx->fcs & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 11;
        return FRM_OK;
     
    case 11:
        if (byte != ((ctx->fcs >> 8) & 0xff)){
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 12;
        return FRM_OK;
    case 12:
        if (byte != 0x16){
            ctx->step = 0;
            return -1;
        }
        return 1;//接收到完整的一帧
    }

    assert(0);
    return -1;
}


int fr_pack_698_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;

    for(int i = 0; i < 3; i ++){
        fifoBuf_putByte(fifo, 0xfe);
    }
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;

    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698){
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_tx_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698){
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    fifo_popData(ctx->apdu, mesg->len, __pack_apdu_byte, pipeline);
    return 1;
}

int fr_pack_698_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    ctx->fcs = get_crc16(ctx->fcs);
    fifoBuf_putByte(fifo, ctx->fcs & 0xff);
    ctx->tx_len++;
    fifoBuf_putByte(fifo, ((ctx->fcs >> 8) & 0xff));
    ctx->tx_len++;
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len++;
    return 1;
}

static void __fr_rx_698_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    uint8_t tail = 0;

    uint16_t crc = fifoBuf_getByte(ififo) + (fifoBuf_getByte(ififo) << 8);//fcs-0
    assert(ctx->fcs == crc);
    tail = fifoBuf_getByte(ififo);
    //请排查消耗数据map长度是否正确
    assert(tail == 0x16);
}

int fr_rx_698_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    __fr_rx_698_check(instance, pipeline, mesg);

    fr_pack_698_head(instance, pipeline, mesg);
    if(mesg->len)
        fr_pack_698_apdu(instance, pipeline, mesg);
    
    fr_pack_698_tail(instance, pipeline, mesg);
    return 0;
}


int fr_timetag_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint32_t interval_sec = 0;
    uint32_t time_sec = 0;
    uint32_t time_tag_sec = 0;
    int offset = 0;

    ctx->timeTag_valid = 0;	
    if ((ctx->service == LINK_Request) || (ctx->service == SECURITY_Request)){
        return -1;
    }

    switch (ctx->service)
    {
    case CONNECT_Request:
        offset = get_CONNECT_Request_len(instance, pipeline, mesg);
        break;
    case RELEASE_Request:
        offset = 1;
        break;
    case GET_Request:
        offset = get_GET_Request_len(instance, pipeline, mesg);
        break;
    case SET_Request:
    case ACTION_Request:
        offset = get_SET_and_ACTION_Request_len(instance, pipeline, mesg);
        break;
    default:
        return 1;
    }

    if (offset < 0) {
        return offset;
    }
    
    memcpy(ctx->timeTag, apdu_rx + ctx->apdu_rx_offset + offset, 11);
    ctx->timeTag_valid = ctx->timeTag[0];
    if (1 == ctx->timeTag_valid) {
        ctx->timeTag_valid = 2;
        if (false == dl_data_time_check(&ctx->timeTag[1], TIME_WILDCARD)){
          
        }else{
            interval_sec = dl_timeTag_interval(&ctx->timeTag[8], 0);
            time_tag_sec = dl_datetime_to_second(&ctx->timeTag[1], 0);
            time_sec     = rtc_mktime(&instance->time);
            
            //if((time_tag_sec >= time_sec) && 
               if(((time_tag_sec + interval_sec) > time_sec)){
                ctx->timeTag_valid = 1;
            }
        }
    }else{
        ctx->timeTag_valid = 0;
    }
    return 1;
}

int fr_addr_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* mbase_cb;
    uint8_t aa_addrbuf[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
    uint8_t i = 0;
    
    if (2 == get_addr_type(ctx->SA)) {
        return 0;
    }

    //地址判断
    if (0 != SA_LOGIC(ctx->SA)) {
        return 0;
    }
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(NULL == mbase_cb){
        return 0;
    }
#if 1 
    if(0 == get_addr_type(ctx->SA)){
        if (0 != mem_compare(ctx->addr, mbase_cb->tsa, SA_LEN(ctx->SA) + 1)){
            if(0 != mem_compare(ctx->addr, aa_addrbuf, 6)){
                return 0;
            }
        }
    }else if(1 == get_addr_type(ctx->SA)){
        for (i = 0; i < (SA_LEN(ctx->SA) + 1); i++)
        {
            if ((ctx->addr[i] & 0xf0) != 0xa0){
                if ((mbase_cb->tsa[i] & 0xf0) != (ctx->addr[i]  & 0xf0)){
                    return 0;
                }
            }
            if ((ctx->addr[i] & 0x0f) != 0x0a){
                if ((mbase_cb->tsa[i] & 0x0f) != (ctx->addr[i]  & 0x0f)){
                    return 0;
                }
            }
        }
        memcpy(ctx->addr, mbase_cb->tsa, SA_LEN(ctx->SA) + 1);
    }else if(3 == get_addr_type(ctx->SA)){
        if(!((1 == (SA_LEN(ctx->SA) + 1)) && (ctx->addr[0] == 0xaa))){
            return 0;
        }
    }else{
        return 0;
    }
#endif
    if (1 == get_addr_type(ctx->SA)) {
        ctx->SA &= 0x3F;
    }
    return 1;
}

int fr_pack_698_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }

    if ((ctx->ctrl & (1 << 7)) == 1){   //合法性和一致性判断
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }
    //控制域判断
    if (!((0 == DIR(ctx->ctrl)) && (1 == PRM(ctx->ctrl)))) {
        return 0;
    }

    if (is_cmd(ctx->ctrl)) {
        return 0;
    }
    
    if(0 == fr_addr_handle(instance, pipeline, mesg)){
        return 0;
    }

    switch(pipeline->id)
    {
        case pid_ir_rx_698: 
            ctx->channel = chl_ir_698;
            break;
        case pid_rx_698: 
            ctx->channel = chl_RS485_1_698;
            break;
        case pid_plc_rx_698: 
            ctx->channel = chl_PLC_698;
            break;
        case pid_4852_rx_698: 
            ctx->channel = chl_RS485_2_698;
            break;
        default:
            ctx->channel = chl_ir_698;
    }
    
    if((ctx->channel == chl_ir_698) || (ctx->channel == chl_RS485_1_698))
    {
        if(ctx->channel == chl_ir_698)  mbase_cb->BackLightTmr = 2* (mbase_cb->lcd_autodis_list_num) * (mbase_cb->lcd_autodis_keep_sec);
        mbase_cb->commdisp_ir485 = 2;
    }
    else if(ctx->channel == chl_PLC_698)
    {
        mbase_cb->commdisp_plc = 2;
    }
    
    return 1;
}


int fr_apdu_effective_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{//协议一致性检查正常，检查APDU是否为有效数据

    return 1;
}

int fr_timeTag_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    
    if(ctx->service == SECURITY_Response){
        return 1;
    }
    if(0 == fr_follow_report_optional(instance, pipeline, mesg)){
        p[cache->wr++] = 0x00;
    }
    
    if (ctx->timeTag_valid >= 1){
        memcpy(p+cache->wr, ctx->timeTag, 11);
        cache->wr += 11;
    }else{
        p[cache->wr++] = 0x00;
    }
    
    return 1;
}

int __put_fifo_byte(void* pctx, uint8_t* byte)
{
    struct fifo_t* fifo = (struct fifo_t*)pctx;
    fifoBuf_putByte(fifo, *byte);
    return 0;
}


int __get_apdu_data_len(struct context_698_t* ctx){
    return ctx->min_len - 4 - (SA_LEN(ctx->SA) + 1) - 5;
}

int fr_segments_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t data_len = 0;
    uint16_t s = 0;
   
    ctx->segment_rx = fifoBuf_getByte(fifo);
    ctx->segment_rx += (fifoBuf_getByte(fifo) << 8);
    s = ctx->segment_rx;
    if(is_first_segment(s)|| is_middle_segment(s) || is_last_segment(s)){
        data_len = __get_apdu_data_len(ctx) - 2;
        
        if(is_first_segment(s)){
            ctx->apdu_rx_len = 0;
            ctx->segment_no = 1;
            fifoBuf_getData(fifo, apdu_rx + ctx->apdu_rx_len,  data_len);
            ctx->apdu_rx_len = data_len;   
        }else if((get_segment_no(s) == ctx->segment_no)){
            if((is_middle_segment(s) || is_last_segment(s))){
                ctx->segment_no++;
                if(is_last_segment(s)){
                    ctx->segment_no = 0;
                }
                fifoBuf_getData(fifo, apdu_rx + ctx->apdu_rx_len,  data_len);
                ctx->apdu_rx_len += data_len;
                if(ctx->apdu_rx_len > Max698_apdu_rx){
                     return -1;
                }
            }
        }
        
        if (is_last_segment(ctx->segment_rx)) {
            clr_segment(ctx->ctrl);
            return 1;
        }else{
            ctx->segment_rx = ctx->segment_rx & 0x3fff;
            set_confirm_segment(ctx->segment_rx);
            fifoBuf_putByte(ctx->apdu, ctx->segment_rx & 0x00ff);
            fifoBuf_putByte(ctx->apdu, (ctx->segment_rx >> 8));
            mesg->len = fifoBuf_getUsed(ctx->apdu);
            mesg->from  = pipeline->id;
            mesg->to    = pid_tx_698;
            mesg->ctrl  = ctx->service |(1<<7);
            mesg->qos   = ctx->choice;
            mesg->id    = 0x00;
            fr_rx_698_end(instance, pipeline, mesg);
            return 0;
        }
    }
    return -1;//异常
}

int handle_scrambler(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int i = 0;
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    
    if (is_scrambler_set(ctx->ctrl)){
        for(i = 0; i<ctx->apdu_rx_len; i++){
            apdu_rx[i] = apdu_rx[i] - 0x33;
        }
    }
    return 1;
}

int __fr_send_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t data[2]={0};
    
    if (fifoBuf_getUsed(ctx->apdu)  > 0){
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512){
            set_segment(ctx->ctrl);
            if(0 == ctx->segment_tx){
                ctx->segment_tx_no = 0;
                ctx->segment_tx = 1;
                set_first_segment_no(ctx->segment_tx_no);
            }else{
                set_middle_segment_no(ctx->segment_tx_no);
            }
            mesg->len = 512;
        } else {
            if(ctx->segment_tx){
                set_segment(ctx->ctrl);
                set_last_segment_no(ctx->segment_tx_no);
  
            }
        }

        if(ctx->segment_tx){
            data[0] = ctx->segment_tx_no;
            data[1] = ctx->segment_tx_no>>8;
            fifoBuf_update_rd(ctx->apdu, 2, 0);
            memcpy(ctx->apdu->buf_ptr + ctx->apdu->rd, data, 2);
        }
        fr_rx_698_end(instance, pipeline, mesg);
        if (mesg->len < 512){
            ringbuffer_clear(ctx->apdu);
            ctx->segment_tx_no = 0;
            ctx->segment_tx = 0;
        }
    }
    return 0;
}


int __fr_segment_tx_handle(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t type = 0;
    uint16_t segment_no = 0;
    uint16_t segment = 0;
    if((is_segment_set(ctx->ctrl)) && (ctx->segment_tx)){
        fifoBuf_getData(fifo, apdu_rx,  2);
        handle_scrambler(instance, pipeline, mesg);
        
        segment = apdu_rx[0] + (apdu_rx[1]<< 8);
        type = get_segment_type(segment);
        if(type == 2)
        {
            segment_no = get_segment_no(segment);
            if(segment_no == get_segment_no(ctx->segment_tx_no))
            {
                ctx->segment_tx_no++;
                __fr_send_handle(instance, pipeline, mesg);
                return 1;
            }
        }
    }
    return 0;
}

int fr_check_service_and_choice(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    
    if (((GET_Request == ctx->service) &&((ctx->choice ==2)|| (ctx->choice == 4)))||
        ((SET_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3)))||
        ((ACTION_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3))))
    {
        mesg->oad_n = apdu_rx[ctx->apdu_rx_offset++];
        fifoBuf_putByte(cache, mesg->oad_n);
    }else if (((GET_Request == ctx->service) &&((ctx->choice ==1) || (ctx->choice ==3)))||
             ((SET_Request == ctx->service) && (ctx->choice == 1))||
             ((ACTION_Request == ctx->service) && (ctx->choice == 1)))
    {
        mesg->oad_n = 1;
    }else if((CONNECT_Request == ctx->service) || (RELEASE_Request == ctx->service) ||
             (REPORT_Response == ctx->service) || (SECURITY_Request == ctx->service)) {
        mesg->oad_n = 0;
    }else{
        return 1;
    }
    
    return 0;
}

int fr_gatekeeper_698_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t data[8]={0};
    struct fifo_t* cache = ctx->apdu;
    int err_no = 0;
    uint8_t service = 0;

    //协议一致性判断和apdu合法判断
    if(0 == fr_pack_698_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    
    fifoBuf_removeData(fifo, APDU_OFFSET(ctx->SA));
    if(__fr_segment_tx_handle(instance, pipeline, mesg)){
        return -1;
    }
    ctx->segment_tx = 0;
    ctx->resp_choice = 0;
    ringbuffer_clear(ctx->apdu);
    ctx->apdu_rx_offset = 0;
    if(is_segment_set(ctx->ctrl)){
        ctx->segment_rx = 0;
        err_no = fr_segments_handle(instance, pipeline,  mesg);
        if (err_no <= 0) {
            return err_no;
        }
    }else{
        ctx->apdu_rx_len = __get_apdu_data_len(ctx);
        if(ctx->apdu_rx_len > Max698_apdu_rx){
            return -1;
        }
        fifoBuf_getData(fifo, apdu_rx,  ctx->apdu_rx_len);
        handle_scrambler(instance, pipeline, mesg);
    }

    update_connect_timeout_cnt(instance, pipeline, mesg);

    ctx->service = apdu_rx[ctx->apdu_rx_offset++];
    ctx->security_choice = 0xff;
    ctx->verification_choice = 0xff;
    if ((SECURITY_Request == ctx->service) || (SECURITY_Response == ctx->service)){
        service = ctx->service;
        err_no = fr_security_request(instance, pipeline, mesg);
        if(err_no == 0){
            return -1;
        }

        ctx->service = apdu_rx[ctx->apdu_rx_offset++];
    }
    
    if (!((CONNECT_Request == ctx->service) || (RELEASE_Request == ctx->service))){
        ctx->choice = apdu_rx[ctx->apdu_rx_offset++];
    }

    ctx->PIID = apdu_rx[ctx->apdu_rx_offset++];
    ctx->PIID |= (1 << 6);//PIID-ACD

    if(0 == fr_timetag_check(instance, pipeline, mesg)){
        ctx->fr_err = FRM_TIME_TAG_ERR;
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }

    mesg->from  = pipeline->id;//从哪里来
    mesg->to    = pid_tx_698;//要到哪里去
    mesg->ctrl  = ctx->service |(1<<7);
    mesg->qos   = ctx->choice;
    mesg->id    = 0x00;//暂时没用

    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);

    if(fr_check_service_and_choice(instance, pipeline, mesg)){
        ctx->service = ERROR_S_Response;
    }
    if((service == SECURITY_Request) && (err_no < 0)){
        ctx->service = SECURITY_Response;
        service = SECURITY_Response;
    }
    
    if(0 == fr_apdu_effective_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }

    switch (ctx->service)
    {
        case CONNECT_Request:
            err_no = fr_handle_connect(instance, pipeline, mesg);
            break;   
        case RELEASE_Request:
            err_no = fr_handle_disconnect(instance, pipeline, mesg);
            break;
        case GET_Request:  
            err_no = fr_handle_get_request(instance, pipeline, mesg);
            break;
        case SET_Request:
        case ACTION_Request:
            err_no = fr_handle_set_action_request(instance, pipeline, mesg);
            break;
        case REPORT_Response://8上报应答 client
            err_no = fr_handle_report_response(instance, pipeline, mesg);
            break;
        case SECURITY_Response:
            cache->buf_ptr[0] = SECURITY_Response;
            cache->buf_ptr[1] = 2;
            cache->buf_ptr[2] = err_no;
            cache->buf_ptr[3] = 0;
            cache->wr = 4;
            break;
        default:
            cache->buf_ptr[0] = ERROR_S_Response;
            cache->buf_ptr[2] = 2;
            break;
    }

    err_no = fr_timeTag_optional(instance, pipeline, mesg);
    if(err_no < 0){
        return err_no;
    }
    
    if (service == SECURITY_Request){
        err_no = fr_security_encode(instance, pipeline, mesg);
        if(err_no < 0){
            cache->buf_ptr[0] = SECURITY_Response;
            cache->buf_ptr[1] = 2;
            cache->buf_ptr[2] = DAR_OTHERS_ERROR;
            cache->buf_ptr[3] = 0;
            cache->wr = 4;
        }else{
            uint8_t len = 2;
            data[0] = SECURITY_Response;
            data[1] = ctx->security_choice;
            len += axdr_len_encode(data+len, ctx->security_data_len);
            cache->rd = Max698_apdu - len;
            memcpy(cache->buf_ptr + cache->rd, data, len);
        }
    }

    if( (0 == get_addr_type(ctx->SA)) || 
        ((1 == get_addr_type(ctx->SA)) && (0 == ctx->resp_choice)) ){
        __fr_send_handle(instance, pipeline, mesg);
        return 0;
    }
    __fr_rx_698_check(instance, pipeline, mesg); 
    return 0;
}

int fr_gatekeeper_698_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    //todo 合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 0){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }

    if (is_segment_set(ctx->ctrl)) { //分帧标志位
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        if (is_last_segment(ctx->segment) > 0) { //收到的是最后帧
            //收齐了  解析 todo
            //get-next
        }else{
            //发送分帧传输的确认帧
        }
        return 0;
    }else{
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
    }
    fifoBuf_removeData(fifo, APDU_OFFSET(ctx->SA));

    ctx->service = fifoBuf_getByte(fifo);
    ctx->choice = fifoBuf_getByte(fifo);
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    fifoBuf_removeData(fifo, (fifo->wr - fifo->rd));

    return 0;
#if 0
    fifoBuf_removeData(fifo, ctx->min_len + 2);
    return 0;
    fifoBuf_removeData(fifo, APDU_OFFSET(ctx->SA));
    uint8_t service = fifoBuf_getByte(fifo);
    uint8_t type = fifoBuf_getByte(fifo);

    mesg->from = pipeline->id;//从哪里来
    mesg->to = pid_tx_698;//要到哪里去
    mesg->ctrl = service | (1 << 7);
    mesg->qos = type;
    mesg->id = 0x00;//暂时没用
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    struct fifo_t* cache = ctx->apdu;
    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);
    if (type == 1){
        mesg->oad_n = 0;
    }else{
        mesg->oad_n = fifoBuf_getByte(fifo);
        fifoBuf_putByte(cache, mesg->oad_n);
    }

    switch (service)
    {
        case 0x85:
            fr_handle_get_request(instance, pipeline, mesg);
            break;
        case 0x86:
        case 0x87:
            fr_handle_set_action_request(instance, pipeline, mesg);
            break;
        case 0x02://connect
        case 0x03://release
            fr_handle_get_request(instance, pipeline, mesg);
            break;
        case 0x08://上报应答 client
            break;
        case 136://上报通知 server
            break;
        case 110://异常响应client
            break;
        case 238://异常响应server
            break;
        case 144://安全响应
            break;
    }

    if (fifoBuf_getUsed(ctx->apdu) > 0){
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512){
            set_segment(ctx->ctrl);
            set_first_segment_no(ctx->segment);//第一帧
            mesg->len = 512;
        }

        fr_rx_698_end(instance, pipeline, mesg);
    }
    return 0;
#endif
}


int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx->step = 0;
    int r = ringbuffer_filter(fifo, __matcher_698, ctx);
    if (r > 0){
        assert(r == ctx->min_len + 2);
#ifdef WIN32
        log_hex(fifo->buf_ptr + fifo->rd, fifo->wr - fifo->rd);
#endif
        return 1;
    }
    return r;
}

int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo_out = (struct fifo_t*)pipeline->out_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t payload[4];

    //client  或者 主动上报 或者 延时读取发送
    if (recv_mesg_v2(instance, pipeline, mesg, payload, 4) >= MESG_V2_OK){

        ctx->ctrl = 0x83;        
        ctx->CA = 0x00;
        ctx->SA = 0x05;
        ctx->addr[0] = 0x08;
        memcpy(ctx->addr, mbase_cb->tsa, SA_LEN(ctx->SA) + 1);
        mesg->from = pid_tx_698;
        mesg->len = (payload[2]<<8) + payload[3] + 2;
        fr_pack_698_head(instance, pipeline, mesg);
        if(mesg->len){
            if(get_report_apdu(instance, pipeline, mesg) > 0){                
                fr_pack_698_apdu(instance, pipeline, mesg);
                if(0 == fr_follow_report_optional(instance, pipeline, mesg)){
                    fifoBuf_putByte(cache, 0);
                }
                fifoBuf_putByte(cache, 0);
            }
        }
        fr_pack_698_tail(instance, pipeline, mesg);
    }

    return 0;
}
