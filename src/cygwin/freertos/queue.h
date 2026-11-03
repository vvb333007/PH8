#ifndef SHIM_FREERTOS_QUEUE_H
#define SHIM_FREERTOS_QUEUE_H
#include "FreeRTOS.h"

typedef struct shim_queue *QueueHandle_t;

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
BaseType_t    xQueueSend(QueueHandle_t q, const void *item, TickType_t ticks);
BaseType_t    xQueueReceive(QueueHandle_t q, void *item, TickType_t ticks);
void          vQueueDelete(QueueHandle_t q);
#endif
