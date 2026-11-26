#ifndef SHIM_FREERTOS_PORTMACRO_H
#define SHIM_FREERTOS_PORTMACRO_H
#include <pthread.h>

/* ESP-IDF style critical section, host version.
 *
 * On the ESP32 portMUX_TYPE is a spinlock and portENTER_CRITICAL() also masks
 * interrupts on the current core; the same core may enter it again (nesting).
 * On the host there are no interrupts, so this is a mutex plus owner/depth,
 * which keeps the nesting behaviour: the owning thread may re-enter freely,
 * every other thread blocks until the owner leaves the outermost level.
 *
 * As in ESP-IDF, the macros take a POINTER:  portENTER_CRITICAL(&aMyLock);
 */
typedef struct {
    pthread_mutex_t aMutex;
    pthread_t       aOwner;   /* meaningful only while nDepth > 0 */
    int             nDepth;   /* nesting depth of the owner, 0 = free */
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED  { PTHREAD_MUTEX_INITIALIZER, 0, 0 }

void vPortMuxInit(portMUX_TYPE *pMux);
void vPortEnterCritical(portMUX_TYPE *pMux);
void vPortExitCritical(portMUX_TYPE *pMux);

#define portMUX_INITIALIZE(pMux)        vPortMuxInit(pMux)
#define spinlock_initialize(pMux)       vPortMuxInit(pMux)

#define portENTER_CRITICAL(pMux)        vPortEnterCritical(pMux)
#define portEXIT_CRITICAL(pMux)         vPortExitCritical(pMux)

/* No ISRs on the host: the ISR / SAFE flavours are the same thing. */
#define portENTER_CRITICAL_ISR(pMux)    vPortEnterCritical(pMux)
#define portEXIT_CRITICAL_ISR(pMux)     vPortExitCritical(pMux)
#define portENTER_CRITICAL_SAFE(pMux)   vPortEnterCritical(pMux)
#define portEXIT_CRITICAL_SAFE(pMux)    vPortExitCritical(pMux)
#define taskENTER_CRITICAL(pMux)        vPortEnterCritical(pMux)
#define taskEXIT_CRITICAL(pMux)         vPortExitCritical(pMux)
#endif
