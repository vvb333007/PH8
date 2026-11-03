#ifndef SHIM_FREERTOS_TASK_H
#define SHIM_FREERTOS_TASK_H
#include "FreeRTOS.h"

typedef void *TaskHandle_t;                 /* handles are not supported (always NULL) */
typedef void (*TaskFunction_t)(void *);

BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack_bytes,
                       void *arg, UBaseType_t prio, TaskHandle_t *out);
void       vTaskDelay(TickType_t ticks);
void       vTaskDelete(TaskHandle_t h);     /* only vTaskDelete(NULL) = exit self */
TickType_t xTaskGetTickCount(void);
#endif
