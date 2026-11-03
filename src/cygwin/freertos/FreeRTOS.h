#ifndef SHIM_FREERTOS_H
#define SHIM_FREERTOS_H
#include <stdint.h>
#include <stddef.h>

typedef int          BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t     TickType_t;

#define pdFALSE        0
#define pdTRUE         1
#define pdPASS         pdTRUE
#define pdFAIL         pdFALSE
#define portMAX_DELAY  ((TickType_t)0xFFFFFFFFu)

/* The host "tick" is 1 ms. */
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#endif
