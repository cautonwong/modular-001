#include "freertos_app.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "time_link.h"

#define APP_TASK_STACK_WORDS 768u
#define APP_TASK_PRIORITY 3u
/* Higher than the app task so a save cannot be preempted by an object
 * mutation; the app task is the only object writer. */
#define WORKER_TASK_PRIORITY (APP_TASK_PRIORITY + 1u)

static TaskHandle_t app_task_handle;
static TaskHandle_t worker_task_handle;
static QueueHandle_t save_queue;
static SemaphoreHandle_t save_done;
static volatile uint8_t critical_pending;

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
    uint8_t item;
    for (;;) {
        if (xQueueReceive(save_queue, &item, portMAX_DELAY) == pdPASS
            && item == SAVE_WORK_ITEM) {
            idis_save_persisted();
            (void)xSemaphoreGive(save_done);
        }
    }
}

void freertos_request_save(void)
{
    uint8_t item = SAVE_WORK_ITEM;
    if (save_queue != NULL) {
        (void)xQueueSend(save_queue, &item, 0);
    }
}

int freertos_take_critical_request(void)
{
    if (critical_pending == 0) {
        return 0;
    }
    critical_pending = 0;
    return 1;
}

int freertos_request_critical_save(uint32_t timeout_ms)
{
    if (save_queue == NULL || save_done == NULL) {
        return -1;
    }
    critical_pending = 1;
    uint8_t item = SAVE_WORK_ITEM;
    (void)xQueueSend(save_queue, &item, 0);

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (critical_pending != 0) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) {
            return -1;
        }
        if (xSemaphoreTake(save_done, deadline - now) != pdPASS) {
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
    save_queue = xQueueCreate(SAVE_QUEUE_DEPTH, sizeof(uint8_t));
    save_done = xSemaphoreCreateBinary();
    if (save_queue == NULL || save_done == NULL
        || xTaskCreate(app_task, "app", APP_TASK_STACK_WORDS, NULL,
                       APP_TASK_PRIORITY, &app_task_handle) != pdPASS
        || xTaskCreate(worker_task, "work", 512, NULL,
                       WORKER_TASK_PRIORITY, &worker_task_handle) != pdPASS) {
        for (;;) {
        }
    }
    vTaskStartScheduler();
    for (;;) {
    }
}
