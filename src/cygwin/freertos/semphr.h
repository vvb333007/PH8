#ifndef SHIM_FREERTOS_SEMPHR_H
#define SHIM_FREERTOS_SEMPHR_H
#include "FreeRTOS.h"

typedef struct shim_sem *SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max, UBaseType_t initial);
SemaphoreHandle_t xSemaphoreCreateBinary(void);   /* starts empty */
SemaphoreHandle_t xSemaphoreCreateMutex(void);    /* non-recursive, starts free */
BaseType_t        xSemaphoreTake(SemaphoreHandle_t s, TickType_t ticks);
BaseType_t        xSemaphoreGive(SemaphoreHandle_t s);
void              vSemaphoreDelete(SemaphoreHandle_t s);
#endif
