#include "nidavellir.h"
#include "type_def.h"
#include "time_link.h"
#include "cpu_surport.h"

typedef struct __ndv_timer
{
    uint8 (*tickfunc)(void *);
    void               *tickpara;
    uint16             timeout;
    uint16             dlyctr;
    uint8              timerid;
    uint8              stoped;
    struct __ndv_timer *pnext;
} ndv_timer;

ndv_timer *gs_timer_link;


ndv_timer link_node[10];

uint8 ndv_timer_create(uint8 (*tickfunc)(void *), void *tickpara, uint16 timeout, uint8 stoped)
{
    ndv_timer *p;
    ndv_timer *q = gs_timer_link, *r;
    uint8     time_id;

    p = gs_timer_link;

    time_id = 0x01;

    while (p != __NULL)                     /**< 循环扫描到最后的timer */
    {
        q       = p;                        /**< 保留上一个 */
        p       = p->pnext;                 /**< 下一个 */
        time_id = q->timerid;              

        if ((q->tickfunc == tickfunc) && (q->tickpara == tickpara))
        {
            return time_id;
        }
    }

    r           =   &link_node[time_id-1];
    r->tickfunc = tickfunc;
    r->tickpara = tickpara;
    r->timeout  = timeout;
    r->dlyctr   = timeout;
    r->pnext    = __NULL;
    r->stoped   = stoped;

    if (gs_timer_link == __NULL)
    {
        gs_timer_link = r;
    }
    else
    {
        q->pnext = r;
    }

    r->timerid = time_id + 1;
    return r->timerid;
}


uint8 ndv_timer_start(uint8 timerid)
{
    ndv_timer *p;

    p = gs_timer_link;

    while (p != __NULL)                     /**< 循环扫描到最后的timer */
    {
        if (p->timerid == timerid)          /**< 该ID的timer已经被创建则返回 */ 
        {
            p->stoped = 0;                  /**< 启动 */
            p->dlyctr = p->timeout + 1;     /**< 防止定时器中途停止后重新启动时,dlyctr 值不等于 timeout. */
            return true;
        }

        p = p->pnext;                       /**< 下一个 */
    }

    return false;                    /**< 返回试图启动不存在的timer */
}


uint8 ndv_timer_stop(uint8 timerid)
{
    ndv_timer *p;

    p = gs_timer_link;

    while (p != __NULL)                      /**< 循环扫描到最后的timer */
    {
        if (p->timerid == timerid)           /**< 该ID的timer已经被创建则返回 */
        {
            p->stoped = 1;                  /**< 停止 */
            return true;
        }
 
        p = p->pnext;                       /**< 下一个 */
    }

    return false;                    /**< 返回试图启动不存在的timer */
}


void ndv_timer_tick(void)
{
    ndv_timer *p = gs_timer_link;

    if (p != __NULL)
    {
        while (p != __NULL)                 /**< 不断循环处理各个timer */
        {
            if (p->stoped == 0)             /**< 判断是否启动 */
            {
                p->dlyctr--;                /**< 倒计时 */

                if (p->dlyctr == 0)         /**< 到0则触发 */
                {
                    p->dlyctr = p->timeout; /**< timer恢复然后调用回调函数 */

                    if (!p->tickfunc(p->tickpara))
                    {
                        p->stoped = 1;      /**< timer被停止了 */
                    }
                }
            }

            p = p->pnext;                   /**< 下一个 */
        }
    }
}


uint8 ndv_timer_start_one(uint8 timerid, uint16 count)
{
    ndv_timer *p;

    p = gs_timer_link;

    while (p != __NULL)                                /**< 循环扫描到最后的timer */
    {
        if (p->timerid == timerid)                     /**< 该ID的timer已经被创建则返回 */
        {
            p->stoped  = 0;                            /**< 启动 */
            p->timeout = count;
            p->dlyctr  = p->timeout + 1;               /**< 防止定时器中途停止后重新启动时,dlyctr 值不等于 timeout. */
            return true;
        }

        p = p->pnext;                                   /**< 下一个 */
    }

    return false;                                      /**< 返回试图启动不存在的timer */
} 

void ndv_timer_init(void)
{
    gs_timer_link = __NULL;
    
    ndv_isr_handle_regsit(ndv_timer_tick , IRQ_SYSTICK);
    
    cm0_systick_init(1);
    
}


/** @} */