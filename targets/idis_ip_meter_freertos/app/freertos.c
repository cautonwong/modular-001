#include "freertos_app.h"
#include "nd_idis_app_context.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "infra/soc/v85xxp/v8500/time_link.h"

#define APP_TASK_STACK_WORDS 768u
#define APP_TASK_PRIORITY 3u
/* Higher than the app task so a save cannot be preempted by an object
 * mutation; the app task is the only object writer. */
#define WORKER_TASK_PRIORITY (APP_TASK_PRIORITY + 1u)

#define SAVE_QUEUE_DEPTH 1u
#define SAVE_WORK_ITEM 1u

uint32_t freertos_uptime_seconds(void)
{
    return (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    for (;;) {
    }
}

static void worker_task(void *argument)
{
    (void)argument;
    nd_idis_app_context_t *app = nd_idis_app_root();
    uint8_t item;
    for (;;) {
        if (xQueueReceive(app->save_queue, &item, portMAX_DELAY) == pdPASS
            && item == SAVE_WORK_ITEM) {
            idis_save_persisted();
            (void)xSemaphoreGive(app->save_done);
        }
    }
}

void freertos_request_save(void)
{
    nd_idis_app_context_t *app = nd_idis_app_root();
    uint8_t item = SAVE_WORK_ITEM;
    if (app->save_queue != NULL) {
        (void)xQueueSend(app->save_queue, &item, 0);
    }
}

int freertos_take_critical_request(void)
{
    nd_idis_app_context_t *app = nd_idis_app_root();
    if (app->critical_pending == 0) {
        return 0;
    }
    app->critical_pending = 0;
    return 1;
}

int freertos_request_critical_save(uint32_t timeout_ms)
{
    nd_idis_app_context_t *app = nd_idis_app_root();
    if (app->save_queue == NULL || app->save_done == NULL) {
        return -1;
    }
    app->critical_pending = 1;
    uint8_t item = SAVE_WORK_ITEM;
    (void)xQueueSend(app->save_queue, &item, 0);

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (app->critical_pending != 0) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) {
            return -1;
        }
        if (xSemaphoreTake(app->save_done, deadline - now) != pdPASS) {
            return -1;
        }
    }
    return 0;
}

static void app_task(void *argument)
{
    (void)argument;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        ndv_timer_tick(NULL);
        idis_app_tick();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}

void freertos_start(void)
{
    nd_idis_app_context_t *app = nd_idis_app_root();
    app->save_queue = xQueueCreate(SAVE_QUEUE_DEPTH, sizeof(uint8_t));
    app->save_done = xSemaphoreCreateBinary();
    if (app->save_queue == NULL || app->save_done == NULL
        || xTaskCreate(app_task, "app", APP_TASK_STACK_WORDS, NULL,
                       APP_TASK_PRIORITY, &app->app_task_handle) != pdPASS
        || xTaskCreate(worker_task, "work", 512, NULL,
                       WORKER_TASK_PRIORITY, &app->worker_task_handle) != pdPASS) {
        for (;;) {
        }
    }
    vTaskStartScheduler();
    for (;;) {
    }
}
