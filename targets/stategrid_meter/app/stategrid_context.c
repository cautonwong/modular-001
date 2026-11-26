#include "targets/stategrid_meter/app/stategrid_context.h"

/* Typed replacements for the legacy pipeline inboxes. Producers append in the
 * first half of a loop pass; the ticks consume one entry per pass, which is
 * the depth the kernel's one-message-per-pass dispatch allowed. */
static struct fifo_t meter_events;
static uint8_t meter_events_buf[16];
static struct fifo_t lowpower_events;
static uint8_t lowpower_events_buf[16];
static struct fifo_t records;
static uint8_t records_buf[64];

void stategrid_context_init(stategrid_context_t *ctx)
{
    fifoBuf_init(&meter_events, meter_events_buf, sizeof(meter_events_buf));
    fifoBuf_init(&lowpower_events, lowpower_events_buf,
                 sizeof(lowpower_events_buf));
    fifoBuf_init(&records, records_buf, sizeof(records_buf));
    ctx->meter_events = &meter_events;
    ctx->lowpower_events = &lowpower_events;
    ctx->records = &records;
    ctx->cfg_connect = default_connect;
    /* The link starts unauthenticated until CONNECT negotiates a mode. */
    ctx->cfg_connect.connect_mode = 0;
    ctx->relay_ui.key_ticks = 0;
    ctx->relay_ui.led_flash = 0;
}

void stategrid_context_set_mbase(stategrid_context_t *ctx,
                                 struct mbase_control_block_t *mbase)
{
    ctx->mbase = mbase;
}

stategrid_board_state_t *stategrid_context_board(void *instance)
{
    stategrid_context_t *ctx = instance;

    return ctx != NULL ? ctx->board : NULL;
}
