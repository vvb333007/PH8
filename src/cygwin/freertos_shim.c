#if defined( __CYGWIN__) || defined(__linux__)

/* Minimal FreeRTOS-on-pthreads shim (tasks, counting semaphores, queues).
*/
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/* time helpers
*/
static void make_deadline(struct timespec *ts, TickType_t ms) {

    clock_gettime(CLOCK_REALTIME, ts);          /* pthread_cond default clock */

    ts->tv_sec  += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;

    if (ts->tv_nsec >= 1000000000L) {

        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

/* Waits on cond with optional absolute deadline. Caller holds mutex */
static int cwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *dl) {

    return dl ? pthread_cond_timedwait(c, m, dl)
              : pthread_cond_wait(c, m);
}

TickType_t xTaskGetTickCount(void) {

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TickType_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* task helpers */

typedef struct {

  TaskFunction_t fn;
  void *arg;

} task_start_t;

/* Trampoline. Pack args and calls fn */
static void *task_tramp(void *p) {

    /* make a copy and free() the source.
    * the reason is that fn() never returns, but calls vTaskDelete() instead. That means we
    * must free(p) before call or we will get a memory leak
    */
    task_start_t s = *(task_start_t *)p; 
    free(p);
    s.fn(s.arg);
    return NULL;
}

BaseType_t xTaskCreate(TaskFunction_t fn,
                       const char    *name,
                       uint32_t       stack_bytes,
                       void          *arg,
                       UBaseType_t    prio,
                       TaskHandle_t  *out) {

    pthread_attr_t attr;
    pthread_t tid;
    int rc;

    /* Suppress warnings */
    name = name;
    prio = prio;

    if (out)
      *out = NULL;
    
    /* Host libc needs far more stack than an ESP32 task; don't mimic ESP sizes. */
    if (stack_bytes < 8 * 1024) {
        stack_bytes = 8 * 1024;
    }

    task_start_t *s = malloc(sizeof(*s));

    if (!s)
      return pdFAIL;
    
    s->fn  = fn;
    s->arg = arg;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_bytes);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    rc = pthread_create(&tid, &attr, task_tramp, s);

    pthread_attr_destroy(&attr);
    if (rc != 0) {
        free(s);
        return pdFAIL;
    }
    return pdPASS;
}

void vTaskDelay(TickType_t ticks) {

    struct timespec ts = {
      .tv_sec = ticks / 1000,
      .tv_nsec = (long)(ticks % 1000) * 1000000L
    };

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

void vTaskDelete(TaskHandle_t h) {

    if (h == NULL) {
        pthread_exit(NULL);
    }
}

/* counting sem */

struct shim_sem {

  pthread_mutex_t m;
  pthread_cond_t  c;
  UBaseType_t     count;
  UBaseType_t     max;
};

SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max, UBaseType_t initial) {

  struct shim_sem *s = calloc(1, sizeof(*s));

  if (!s)
    return NULL;
    
  pthread_mutex_init(&s->m, NULL);
  pthread_cond_init(&s->c, NULL);

  s->max   = max;
  s->count = initial;

  return s;
}

SemaphoreHandle_t xSemaphoreCreateBinary(void) {

  return xSemaphoreCreateCounting(1, 0);
}

SemaphoreHandle_t xSemaphoreCreateMutex(void) {

  return xSemaphoreCreateCounting(1, 1);
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t ticks) {

    struct timespec dl, *pdl = NULL;

    if (ticks != portMAX_DELAY && ticks > 0) {

        make_deadline(&dl, ticks);
        pdl = &dl;
    }

    pthread_mutex_lock(&s->m);

    while (s->count == 0) {

        if (ticks == 0 || cwait(&s->c, &s->m, pdl) == ETIMEDOUT) {

            pthread_mutex_unlock(&s->m);
            return pdFALSE;
        }
    }

    s->count--;
    pthread_mutex_unlock(&s->m);

    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t s) {

    pthread_mutex_lock(&s->m);

    if (s->count >= s->max) {
        pthread_mutex_unlock(&s->m);
        return pdFALSE;
    }

    s->count++;

    pthread_cond_signal(&s->c);
    pthread_mutex_unlock(&s->m);

    return pdTRUE;
}

void vSemaphoreDelete(SemaphoreHandle_t s) {
    if (s) {
        pthread_mutex_destroy(&s->m);
        pthread_cond_destroy(&s->c);
        free(s);
    }
}

/* queue */

struct shim_queue {
    pthread_mutex_t m;
    pthread_cond_t  not_empty, not_full;
    uint8_t        *buf;
    size_t          item, len, head, count;
};

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size) {

    struct shim_queue *q = calloc(1, sizeof(*q));

    if (!q)
        return NULL;
    
    q->buf = malloc((size_t)length * item_size);
    if (!q->buf) {
        free(q);
        return NULL;
    }
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    q->item = item_size;
    q->len  = length;
    return q;
}

BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t ticks)
{
    struct timespec dl, *pdl = NULL;
    if (ticks != portMAX_DELAY && ticks > 0) {
        make_deadline(&dl, ticks);
        pdl = &dl;
    }
    pthread_mutex_lock(&q->m);
    while (q->count == q->len) {
        if (ticks == 0 || cwait(&q->not_full, &q->m, pdl) == ETIMEDOUT) {
            pthread_mutex_unlock(&q->m);
            return pdFALSE;
        }
    }
    size_t tail = (q->head + q->count) % q->len;
    memcpy(q->buf + tail * q->item, item, q->item);
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->m);
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t ticks)
{
    struct timespec dl, *pdl = NULL;
    if (ticks != portMAX_DELAY && ticks > 0) {
        make_deadline(&dl, ticks);
        pdl = &dl;
    }
    pthread_mutex_lock(&q->m);
    while (q->count == 0) {
        if (ticks == 0 || cwait(&q->not_empty, &q->m, pdl) == ETIMEDOUT) {
            pthread_mutex_unlock(&q->m);
            return pdFALSE;
        }
    }
    memcpy(item, q->buf + q->head * q->item, q->item);
    q->head = (q->head + 1) % q->len;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->m);
    return pdTRUE;
}

void vQueueDelete(QueueHandle_t q)
{
    if (q) {
        pthread_mutex_destroy(&q->m);
        pthread_cond_destroy(&q->not_empty);
        pthread_cond_destroy(&q->not_full);
        free(q->buf);
        free(q);
    }
}

/* nDepth and aOwner are read by "foreign" threads without the mutex (to
 * recognise re-entry), so they are accessed with __atomic builtins.
 * Ordering: the owner writes aOwner first, then nDepth (release); a reader
 * loads nDepth (acquire), then aOwner. A thread can only ever see its OWN id
 * in aOwner if it holds the lock: on final exit nDepth goes to 0 before
 * the unlock, and the next owner writes its id before nDepth becomes 1. */

void vPortMuxInit(portMUX_TYPE *pMux) {

    pthread_mutex_init(&pMux->aMutex, NULL);
    pMux->nDepth = 0;
}

void vPortEnterCritical(portMUX_TYPE *pMux) {

    pthread_t aSelf = pthread_self();
    pthread_t aOwner;

    int nDepth = __atomic_load_n(&pMux->nDepth, __ATOMIC_ACQUIRE);
    if (nDepth > 0) {
        __atomic_load(&pMux->aOwner, &aOwner, __ATOMIC_RELAXED);
        if (pthread_equal(aOwner, aSelf)) {
            /* Already ours: just one level deeper. */
            __atomic_store_n(&pMux->nDepth, nDepth + 1, __ATOMIC_RELAXED);
            return;
        }
    }

    pthread_mutex_lock(&pMux->aMutex);

    __atomic_store(&pMux->aOwner, &aSelf, __ATOMIC_RELAXED);
    __atomic_store_n(&pMux->nDepth, 1, __ATOMIC_RELEASE);
}

void vPortExitCritical(portMUX_TYPE *pMux)
{
    pthread_t aOwner;
    int nDepth = __atomic_load_n(&pMux->nDepth, __ATOMIC_ACQUIRE);

    __atomic_load(&pMux->aOwner, &aOwner, __ATOMIC_RELAXED);
    if (nDepth <= 0 || !pthread_equal(aOwner, pthread_self())) {
        /* ESP-IDF asserts here too: exit without a matching enter. */
        fprintf(stderr, "portEXIT_CRITICAL: mux %p not held by this thread\n", (void *)pMux);
        abort();
    }

    if (nDepth > 1) {
        __atomic_store_n(&pMux->nDepth, nDepth - 1, __ATOMIC_RELAXED);
        return;
    }
    __atomic_store_n(&pMux->nDepth, 0, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&pMux->aMutex);
}
#endif /* __CYGWIN__ */
