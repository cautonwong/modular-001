#ifndef ND_DISPLAY_BACKLIGHT_H
#define ND_DISPLAY_BACKLIGHT_H

typedef struct nd_backlight_port {
    void *ctx;
    void (*set)(void *ctx, int on);
} nd_backlight_port_t;

void nd_backlight_bind(const nd_backlight_port_t *port);
void nd_backlight_set(int on);

#endif
