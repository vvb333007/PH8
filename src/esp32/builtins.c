/*
 * PlatformPHP - An ESP32 port and further development of the PH7 PHP Engine
 *
 * Copyright (c) 2026 Viacheslav Logunov
 * SPDX-License-Identifier: GPL
 *
 * Author:
 *   Viacheslav Logunov <vvb333007@gmail.com>
 *
 * Project:
 *   https://github.com/vvb333007/PH7-ESP32
 */

/* This file contains native bindings to the Arduino Core functions + some of ESP-IDF functions
 * Bindings are similar in structure, and are declared in special form: using a template macro (see below)
 *
 * Some Arduino functions has changed their prototypes: if it is convinient, we returnresult as a return value, not 
 * by the reference: e.g. fgets(&result) is implemented as $result = fgets();
 *
 * There are allocations made with malloc(): it is ok, because buffer is duplicated when converted to string
 */
#include "ph7int.h"
#include "builtins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>


/* Templates and boilerplates.
 All the remplates are made of IMPLEMENT.. and Impl macros. There are ,any of them
*/


/*
 *
 *  C11 Dispatcher, choose converter function by the type T 
 */
#define ph7_value_to(T, v) _Generic((T)0,          \
    _Bool:              ph7_value_to_bool,         \
    char:               ph7_value_to_int,          \
    signed char:        ph7_value_to_int,          \
    unsigned char:      ph7_value_to_int,          \
    short:              ph7_value_to_int,          \
    unsigned short:     ph7_value_to_int,          \
    int:                ph7_value_to_int,          \
    unsigned int:       ph7_value_to_int64,        \
    long:               ph7_value_to_int64,        \
    unsigned long:      ph7_value_to_int64,        \
    long long:          ph7_value_to_int64,        \
    unsigned long long: ph7_value_to_int64,        \
    float:              ph7_value_to_double,       \
    double:             ph7_value_to_double        \
)(v)

/* Low level template for foreign functions which can not be generalized into IMPLEMENT_N_N() 
 * E.g. function random() below can work with 1 or 2 arguments - means no suitable IMPLEMENT macro
 */
#define Impl( Name_ , pCtx_ , nArg_, apArg_) \
  int esp32_builtin_ ## Name_ (ph7_context *pCtx_, int nArg_, ph7_value **apArg_)

/* To be used inside IMPLEMENT*() bodies for functions which the current chip does not have
 */
#define NOT_SUPPORTED(name_)                                \
  ph7_context_throw_error(pCtx, PH7_CTX_WARNING,            \
    #name_ ": feature is not supported by this chip");                 \
  ph7_result_null(pCtx)


/* Template for No-arg functions like millis(), micros(), esp_timer_get_time() 
 * Implementation code (i.e. `...`) must call ph7_result_* to set the
 * return value.
 * if it doesnt the return value is whatever was in the accumulator. 
*/
#define IMPLEMENT(name, ...)                                \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    (void)nArg; (void)apArg;                                \
    __VA_ARGS__;                                            \
    return PH7_OK;                                          \
  }


/* One-arg functions like digitalRead() or delay() 
*/
#define IMPLEMENT_N(name, T0, ...)                          \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    if (nArg >= 1 && ph7_value_is_numeric(apArg[0])) {      \
      T0 a0 = (T0)ph7_value_to(T0, apArg[0]);               \
      a0 = a0; \
      __VA_ARGS__;                                          \
    } else {                                                \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,        \
        #name ": expected 1 numeric argument");             \
      ph7_result_null(pCtx);                                \
    }                                                       \
    return PH7_OK;                                          \
  }



/* Two numeric arguments like digitalWrite(pin, value).
 */
#define IMPLEMENT_N_N(name, T0, T1, ...)                    \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    if (nArg >= 2 &&                                        \
        ph7_value_is_numeric(apArg[0]) &&                   \
        ph7_value_is_numeric(apArg[1])) {                   \
      T0 a0 = (T0)ph7_value_to(T0, apArg[0]);               \
      T1 a1 = (T1)ph7_value_to(T1, apArg[1]);               \
      a0 = a0; \
      a1 = a1; \
      __VA_ARGS__;                                          \
    } else {                                                \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,        \
        #name ": expected 2 numeric arguments");            \
      ph7_result_null(pCtx);                                \
    }                                                       \
    return PH7_OK;                                          \
  }

/* Three numeric arguments like ledcAttach(pin, freq, resolution).
 * Same contract as IMPLEMENT_N_N(): a0, a1, a2 are available to the code.
 */
#define IMPLEMENT_N_N_N(name, T0, T1, T2, ...)              \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    if (nArg >= 3 &&                                        \
        ph7_value_is_numeric(apArg[0]) &&                   \
        ph7_value_is_numeric(apArg[1]) &&                   \
        ph7_value_is_numeric(apArg[2])) {                   \
      T0 a0 = (T0)ph7_value_to(T0, apArg[0]);               \
      T1 a1 = (T1)ph7_value_to(T1, apArg[1]);               \
      T2 a2 = (T2)ph7_value_to(T2, apArg[2]);               \
      a0 = a0; \
      a1 = a1; \
      a2 = a2; \
      __VA_ARGS__;                                          \
    } else {                                                \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,        \
        #name ": expected 3 numeric arguments");            \
      ph7_result_null(pCtx);                                \
    }                                                       \
    return PH7_OK;                                          \
  }

/* Four numeric arguments like shiftOut(data, clock, order, value).
 * a0, a1, a2, a3 are available to the code.
 */
#define IMPLEMENT_N_N_N_N(name, T0, T1, T2, T3, ...)        \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    if (nArg >= 4 &&                                        \
        ph7_value_is_numeric(apArg[0]) &&                   \
        ph7_value_is_numeric(apArg[1]) &&                   \
        ph7_value_is_numeric(apArg[2]) &&                   \
        ph7_value_is_numeric(apArg[3])) {                   \
      T0 a0 = (T0)ph7_value_to(T0, apArg[0]);               \
      T1 a1 = (T1)ph7_value_to(T1, apArg[1]);               \
      T2 a2 = (T2)ph7_value_to(T2, apArg[2]);               \
      T3 a3 = (T3)ph7_value_to(T3, apArg[3]);               \
      a0 = a0; \
      a1 = a1; \
      a2 = a2; \
      a3 = a3; \
      __VA_ARGS__;                                          \
    } else {                                                \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,        \
        #name ": expected 4 numeric arguments");            \
      ph7_result_null(pCtx);                                \
    }                                                       \
    return PH7_OK;                                          \
  }

/* Five numeric arguments like uartSetPins(). a0 .. a4 are available to the code */
#define IMPLEMENT_N_N_N_N_N(name, T0, T1, T2, T3, T4, ...)  \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    if (nArg >= 5 &&                                        \
        ph7_value_is_numeric(apArg[0]) &&                   \
        ph7_value_is_numeric(apArg[1]) &&                   \
        ph7_value_is_numeric(apArg[2]) &&                   \
        ph7_value_is_numeric(apArg[3]) &&                   \
        ph7_value_is_numeric(apArg[4])) {                   \
      T0 a0 = (T0)ph7_value_to(T0, apArg[0]);               \
      T1 a1 = (T1)ph7_value_to(T1, apArg[1]);               \
      T2 a2 = (T2)ph7_value_to(T2, apArg[2]);               \
      T3 a3 = (T3)ph7_value_to(T3, apArg[3]);               \
      T4 a4 = (T4)ph7_value_to(T4, apArg[4]);               \
      a0 = a0; \
      a1 = a1; \
      a2 = a2; \
      a3 = a3; \
      a4 = a4; \
      __VA_ARGS__;                                          \
    } else {                                                \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,        \
        #name ": expected 5 numeric arguments");            \
      ph7_result_null(pCtx);                                \
    }                                                       \
    return PH7_OK;                                          \
  }


#ifndef PH7_DISABLE_ESP32_ARDUINO_FUNC
# ifdef ESP32
#  include <Arduino.h>
#  include "esp32-hal-timer.h"
#  include "esp32-hal-uart.h"
#  include "esp32-hal-spi.h"
#  include "esp32-hal-rmt.h"
# endif

/*
 * Handle tables
 *
 * Some Arduino Core objects are pointers (hw_timer_t *, uart_t *, spi_t *). PH8 can hold a pointer
 * in its int64 container but this is dangerous. Holding a pointer as resource creates unnecessary overhead;
 * plus I am too lazy to do it that way. Another problems with resource-type is that it is local and thus
 * it will be difficult task to make shared access. An UART initialized by one VM should not be initialized
 * again by another VM
 *
 * So instead we use indicies in so-called Handle Tables. Handle tables are global tables, shared by all VM
 * and all engines.  * A timer started by one script is visible (and can be stopped) by any other.
 * Table sizes come from SOC_* macros. A pointer returned by the Arduino Core is placed in the Handle Table and its index is
 * returned instead
 *
 *   $t = timerBegin(1000000);   // returns 0, 1, 2 ...  (index is chosen by PH8)
 *   $u = uartBegin(1, 115200);  // returns 1            (index is the UART number. i.e. choosen by User)
 *   $s = spiStartBus(HSPI, ..); // returns HSPI         (index is the bus number, by the User)
 *
 * Every timer...($t) / uart...($u) / spi...($s) function turns the index back
 * into the pointer.
 *
 * Invalid, closed or never opened handle throws warning, not crashes.
 *
 * Every slot in handletable cycles through :  FREE (NULL) -> PENDING -> handle -> PENDING -> FREE
 * PENDING means "reserved, a core call is in progress". This is to move actual calls out of the lock.
 *
 */


#define HT_PENDING   ((void *)1)   /* Fake address, to signal that slot is reserved reserved */


#ifdef __CYGWIN__
/* PC build: use placeholders instead of real numbers. we are on PC*/
# define HT_FAKE      ((void *)2)   /* Fake handle on Cygwin */

# ifndef SOC_TIMER_GROUPS
#  define SOC_TIMER_GROUPS 2
# endif
# ifndef SOC_TIMER_GROUP_TIMERS_PER_GROUP
#  define SOC_TIMER_GROUP_TIMERS_PER_GROUP 2
# endif
# ifndef SOC_UART_NUM
#  define SOC_UART_NUM 3
# endif
# ifndef SOC_SPI_PERIPH_NUM
#  define SOC_SPI_PERIPH_NUM 3
# endif
/* mock esp-idf structures (we only use pointers) */
typedef struct esp32_fake_timer hw_timer_t;
typedef struct esp32_fake_uart  uart_t;
typedef struct esp32_fake_spi   spi_t;
/* No locking on Cygwin */
# define ESP32_LOCK()
# define ESP32_UNLOCK()
#else
/* ESP32 Build: */
# if !SOC_GPTIMER_SUPPORTED || !SOC_UART_SUPPORTED || !SOC_GPSPI_SUPPORTED
#  error "hardware timers, UART and GP-SPI are required (soc/soc_caps.h)"
# endif
/* Tables lock. */
static portMUX_TYPE HandleLock = portMUX_INITIALIZER_UNLOCKED;
# define ESP32_LOCK()    portENTER_CRITICAL(&HandleLock)
# define ESP32_UNLOCK()  portEXIT_CRITICAL(&HandleLock)
#endif

#define ESP32_TIMER_MAX   (SOC_TIMER_GROUPS * SOC_TIMER_GROUP_TIMERS_PER_GROUP)
#define ESP32_UART_MAX    (SOC_UART_NUM)
/* Arduino's SPI buses 0..N-1, but on the original ESP32 they are 1..3 (FSPI, HSPI, VSPI):
 * one slot more. Bad bus nr rejected by spiStartBus()
 */
#define ESP32_SPI_MAX     (SOC_SPI_PERIPH_NUM + 1)

/* Generic slot operations.ht_ -> handle table */

/* Reserve a free slot, return its index or -1 if there are no free slots
 * Slot Number is choosen automatically
 */
static int ht_reserve(void **apTable, int nCount) {
  int i, iFound = -1;
  ESP32_LOCK();
  for (i = 0; i < nCount; i++) {
    if (apTable[i] == NULL) {
      apTable[i] = HT_PENDING;
      iFound = i;
      break;
    }
  }
  ESP32_UNLOCK();
  return iFound;
}

/* Open slot iIdx: for tables where the caller picks the peripheral (UART number, SPI bus
 * number). Returns 1 on success and stores the previous handle (NULL if the slot was
 * free) into *ppPrev
 * Return 0 if the index is bad
 * Must be followed by ht_commit()
 */
static int ht_open(void **apTable, int nCount, int iIdx, void **ppPrev) {

  int bOk = 0;

  *ppPrev = NULL;

  if (iIdx >= 0 && iIdx < nCount) {

    ESP32_LOCK();
    if (apTable[iIdx] != HT_PENDING) {

      *ppPrev = apTable[iIdx];
      apTable[iIdx] = HT_PENDING;
      bOk = 1;

    }
    ESP32_UNLOCK();
  }
  return bOk;
}

/* Handle by index or NULL if the index is bad / slot is free / slot is pending 
*/
static void *ht_get(void **apTable, int nCount, int iIdx) {

  void *pHandle = NULL;

  if (iIdx >= 0 && iIdx < nCount) {

    ESP32_LOCK();
    pHandle = apTable[iIdx];
    ESP32_UNLOCK();

    if (pHandle == HT_PENDING)
      pHandle = NULL;
  }
  return pHandle;
}

/* Same as get, but marks the slot PENDING: caller is going to close the handle.
 * Must be followed by ht_commit(..., NULL)
 */
static void *ht_get_for_close(void **apTable, int nCount, int iIdx) {
  void *pHandle = NULL;
  if (iIdx >= 0 && iIdx < nCount) {
    ESP32_LOCK();
    pHandle = apTable[iIdx];
    if (pHandle != NULL && pHandle != HT_PENDING)
      apTable[iIdx] = HT_PENDING;
    else
      pHandle = NULL;
    ESP32_UNLOCK();
  }
  return pHandle;
}

/* Store the handle into a reserved/opened/taken slot. NULL frees the slot */
static void ht_commit(void **apTable, int iIdx, void *pHandle) {
  ESP32_LOCK();
  apTable[iIdx] = pHandle;
  ESP32_UNLOCK();
}

/* The tables */
static void *apTimer[ESP32_TIMER_MAX];   /* hw_timer_t *, slot is picked by timerBegin() */
static void *apUart[ESP32_UART_MAX];     /* uart_t *,     slot is the UART number */
static void *apSpi[ESP32_SPI_MAX];       /* spi_t *,      slot is the SPI bus number */

#define esp32_timer_get(iIdx_) ((hw_timer_t *)ht_get(apTimer, ESP32_TIMER_MAX, iIdx_))
#define esp32_uart_get(iIdx_) ((uart_t *)    ht_get(apUart,  ESP32_UART_MAX, iIdx_))
#define esp32_spi_get(iIdx_) ((spi_t *)     ht_get(apSpi,   ESP32_SPI_MAX, iIdx_))


/* Functions with a handle as the first argument.
 *
 *  IMPLEMENT_H     (name, TH, getter, code)                  name($h)
 *  IMPLEMENT_H_N   (name, TH, getter, T1, code)              name($h, $a1)
 *  IMPLEMENT_H_N_N (name, TH, getter, T1, T2, code)           name($h, $a1, $a2)
 *  IMPLEMENT_H_N_N_N(name, TH, getter, T1, T2, T3, code)      name($h, $a1, $a2, $a3)
 *  IMPLEMENT_H_S   (name, TH, getter, code)                  name($h, string $data)
 *
 * TH is the handle type (hw_timer_t, uart_t, spi_t), getter is esp32_xxx_get().
 * The code is only run for a valid handle and valid arguments, and sees:
 *    pHandle      - the pointer (TH *)
 *    a0           - the handle index (for the PC stubs)
 *    a1, a2, a3   - numeric arguments, converted to T1, T2, T3
 *    zData, nLen  - IMPLEMENT_H_S only: the string argument (binary safe!)
 */
#define IMPLEMENT_H(name, TH, getter, ...)                              \
  Impl(name, pCtx, nArg, apArg) {                                       \
    TH *pHandle = NULL;                                                 \
    if (nArg >= 1 && ph7_value_is_numeric(apArg[0]))                    \
      pHandle = getter(ph7_value_to_int(apArg[0]));                     \
    if (pHandle != NULL) {                                              \
      int a0 = ph7_value_to_int(apArg[0]);                              \
      a0 = a0; pHandle = pHandle;                                       \
      __VA_ARGS__;                                                      \
    } else {                                                            \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                    \
        #name ": invalid handle");                                      \
      ph7_result_null(pCtx);                                            \
    }                                                                   \
    return PH7_OK;                                                      \
  }

#define IMPLEMENT_H_N(name, TH, getter, T1, ...)                        \
  Impl(name, pCtx, nArg, apArg) {                                       \
    TH *pHandle = NULL;                                                 \
    if (nArg >= 2 &&                                                    \
        ph7_value_is_numeric(apArg[0]) &&                               \
        ph7_value_is_numeric(apArg[1]))                                 \
      pHandle = getter(ph7_value_to_int(apArg[0]));                     \
    if (pHandle != NULL) {                                              \
      int a0 = ph7_value_to_int(apArg[0]);                              \
      T1  a1 = (T1)ph7_value_to(T1, apArg[1]);                          \
      a0 = a0; a1 = a1; pHandle = pHandle;                              \
      __VA_ARGS__;                                                      \
    } else {                                                            \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                    \
        #name ": invalid handle or arguments");                         \
      ph7_result_null(pCtx);                                            \
    }                                                                   \
    return PH7_OK;                                                      \
  }

#define IMPLEMENT_H_N_N(name, TH, getter, T1, T2, ...)                  \
  Impl(name, pCtx, nArg, apArg) {                                       \
    TH *pHandle = NULL;                                                 \
    if (nArg >= 3 &&                                                    \
        ph7_value_is_numeric(apArg[0]) &&                               \
        ph7_value_is_numeric(apArg[1]) &&                               \
        ph7_value_is_numeric(apArg[2]))                                 \
      pHandle = getter(ph7_value_to_int(apArg[0]));                     \
    if (pHandle != NULL) {                                              \
      int a0 = ph7_value_to_int(apArg[0]);                              \
      T1  a1 = (T1)ph7_value_to(T1, apArg[1]);                          \
      T2  a2 = (T2)ph7_value_to(T2, apArg[2]);                          \
      a0 = a0; a1 = a1; a2 = a2; pHandle = pHandle;                     \
      __VA_ARGS__;                                                      \
    } else {                                                            \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                    \
        #name ": invalid handle or arguments");                         \
      ph7_result_null(pCtx);                                            \
    }                                                                   \
    return PH7_OK;                                                      \
  }

#define IMPLEMENT_H_N_N_N(name, TH, getter, T1, T2, T3, ...)            \
  Impl(name, pCtx, nArg, apArg) {                                       \
    TH *pHandle = NULL;                                                 \
    if (nArg >= 4 &&                                                    \
        ph7_value_is_numeric(apArg[0]) &&                               \
        ph7_value_is_numeric(apArg[1]) &&                               \
        ph7_value_is_numeric(apArg[2]) &&                               \
        ph7_value_is_numeric(apArg[3]))                                 \
      pHandle = getter(ph7_value_to_int(apArg[0]));                     \
    if (pHandle != NULL) {                                              \
      int a0 = ph7_value_to_int(apArg[0]);                              \
      T1  a1 = (T1)ph7_value_to(T1, apArg[1]);                          \
      T2  a2 = (T2)ph7_value_to(T2, apArg[2]);                          \
      T3  a3 = (T3)ph7_value_to(T3, apArg[3]);                          \
      a0 = a0; a1 = a1; a2 = a2; a3 = a3; pHandle = pHandle;            \
      __VA_ARGS__;                                                      \
    } else {                                                            \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                    \
        #name ": invalid handle or arguments");                         \
      ph7_result_null(pCtx);                                            \
    }                                                                   \
    return PH7_OK;                                                      \
  }

#define IMPLEMENT_H_S(name, TH, getter, ...)                            \
  Impl(name, pCtx, nArg, apArg) {                                       \
    TH *pHandle = NULL;                                                 \
    if (nArg >= 2 &&                                                    \
        ph7_value_is_numeric(apArg[0]) &&                               \
        ph7_value_is_string(apArg[1]))                                  \
      pHandle = getter(ph7_value_to_int(apArg[0]));                     \
    if (pHandle != NULL) {                                              \
      int a0 = ph7_value_to_int(apArg[0]);                              \
      int nLen = 0;                                                     \
      const char *zData = ph7_value_to_string(apArg[1], &nLen);         \
      a0 = a0; nLen = nLen; zData = zData; pHandle = pHandle;           \
      __VA_ARGS__;                                                      \
    } else {                                                            \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                    \
        #name ": invalid handle or arguments (string expected)");       \
      ph7_result_null(pCtx);                                            \
    }                                                                   \
    return PH7_OK;                                                      \
  }


/* Actual implemetation.
 * IMPLEMENT_N_N(functionName, arg1 type, arg2 type, CODE...) will create a function
 * int ep32_builtin_functionName(...) which will be called each time PHP codes calls 
 * functionName($arg1, $arg2)
 *
 * The `...` part is the actual code to be executed;
 * Arguments are available as a0, a1, a2, and so on
 */



// function pinMode(int $pin, int $mode): void
//
IMPLEMENT_N_N(pinMode, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"pinMode(%d, %x)\r\n",a0,a1);
#else
  pinMode(a0, a1);
  /* No return value */
#endif
)

// function digitalWrite
//
IMPLEMENT_N_N(digitalWrite, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalWrite(%d, %d)\r\n",a0, a1);
#else
  digitalWrite(a0, a1 ? 1 : 0);
#endif
)

// function delay
//
IMPLEMENT_N(delay, int,
#ifdef __CYGWIN__
  fprintf(stderr,"delay(%d)\r\n",a0);
#else
  delay(a0);
#endif
)

// function delayMicroseconds
//
IMPLEMENT_N(delayMicroseconds, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"delayMicroseconds(%lld)\r\n",a0);
#else
  delayMicroseconds(a0);
#endif
)

// function digitalRead
//
IMPLEMENT_N(digitalRead, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalRead(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, digitalRead(a0));
#endif
)

// function millis
//
IMPLEMENT(millis,
#ifdef __CYGWIN__
  fprintf(stderr,"millis()\r\n");
#else
  ph7_result_int64(pCtx, (sxi64)millis());
#endif
)

// function micros
//
IMPLEMENT(micros,
#ifdef __CYGWIN__
  fprintf(stderr,"micros()\r\n");
#else
  ph7_result_int64(pCtx, (sxi64)micros());
#endif
)

// bool $ret = digitalPinIsValid($pin)
//
IMPLEMENT_N(digitalPinIsValid, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinIsValid(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, digitalPinIsValid(a0));
#endif
)

// bool digitalPinCanOutput($pin)
//
IMPLEMENT_N(digitalPinCanOutput, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinCanOutput(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, digitalPinCanOutput(a0));
#endif
)


// $rtc_pin = digitalPinToRtcPin($pin)
//
IMPLEMENT_N(digitalPinToRtcPin, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinToRtcPin(%d)\r\n",a0);
#else
#ifdef RTC_GPIO_IS_VALID_GPIO
  ph7_result_int(pCtx, digitalPinToRtcPin(a0));
#endif
#endif
)




// $chan = digitalPinToDacChannel($pin)
//
IMPLEMENT_N(digitalPinToDacChannel, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinToDacChannel(%d)\r\n",a0);
#else
#ifdef DAC_CHANNEL_1_GPIO_NUM
  ph7_result_int(pCtx, digitalPinToDacChannel(a0));
#endif
#endif
)


// $chan = digitalPinToTouchChannel($pin)
//
IMPLEMENT_N(digitalPinToTouchChannel, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinToTouchChannel(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, digitalPinToTouchChannel(a0));
#endif
)

// $chan = digitalPinToAnalogChannel($pin)
//
IMPLEMENT_N(digitalPinToAnalogChannel, int,
#ifdef __CYGWIN__
  fprintf(stderr,"digitalPinToAnalogChannel(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, digitalPinToAnalogChannel(a0));
#endif
)

// $digi_pin = analogChannelToDigitalPin($ana_pin);
//
IMPLEMENT_N(analogChannelToDigitalPin, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogChannelToDigitalPin(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, analogChannelToDigitalPin(a0));
#endif
)

// $value = shiftIn(int $data_pin, int $clock_pin, int $bit_order): int
// $bit_order is LSBFIRST (0) or MSBFIRST (1)
//
IMPLEMENT_N_N_N(shiftIn, int, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"shiftIn(%d, %d, %d)\r\n",a0,a1,a2);
#else
  ph7_result_int(pCtx, shiftIn(a0, a1, a2));
#endif
)

// shiftOut(int $data_pin, int $clock_pin, int $bit_order, int $value): void
//
IMPLEMENT_N_N_N_N(shiftOut, int, int, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"shiftOut(%d, %d, %d, %d)\r\n",a0,a1,a2,a3);
#else
  shiftOut(a0, a1, a2, a3);
#endif
)

/* pulseIn() and pulseInLong() share the same argument parsing:
 *
 * Return value is the pulse length in microseconds, 0 if timed out.
 * Both functions block the VM until the pulse is over (or the timeout expires)
 */
static int __pulseIn(ph7_context *pCtx, int nArg, ph7_value **apArg, int bLong, const char *zName, const char *zError) {

  sxi64 nTimeout = 1000000;

  if (nArg < 2 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      (nArg >= 3 && !ph7_value_is_numeric(apArg[2]))) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, zError);
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int iPin   = ph7_value_to_int(apArg[0]);
  const int iState = ph7_value_to_int(apArg[1]) ? 1 : 0;

  if (nArg >= 3)
    nTimeout = ph7_value_to_int64(apArg[2]);
  if (nTimeout < 0)
    nTimeout = 0;

  (void)bLong; (void)zName; (void)iPin; (void)iState;

#ifdef __CYGWIN__
  fprintf(stderr,"%s(%d, %d, %lld)\r\n", zName, iPin, iState, (long long)nTimeout);
#else
  ph7_result_int64(pCtx, (sxi64)(bLong ? pulseInLong(iPin, iState, (unsigned long)nTimeout)
                                       : pulseIn(iPin, iState, (unsigned long)nTimeout)));
#endif
  return PH7_OK;
}

// function pulseIn(int $pin, int $state, int $timeout_us = 1000000): int
//
Impl(pulseIn, pCtx, nArg, apArg) {
  return __pulseIn(pCtx, nArg, apArg, 0, "pulseIn", "pulseIn: expected (int $pin, int $state [, int $timeout_us])");
}

// function pulseInLong(int $pin, int $state, int $timeout_us = 1000000): int
//
Impl(pulseInLong, pCtx, nArg, apArg) {
  return __pulseIn(pCtx, nArg, apArg, 1, "pulseInLong", "pulseInLong: expected (int $pin, int $state [, int $timeout_us])");
}

// $res = makeWord($w)         : low 16 bits of $w
// $res = makeWord($high, $low): ($high << 8) | $low, both are bytes
// Pure computation, no hardware involved
//
Impl(makeWord, pCtx, nArg, apArg) {

  if (nArg == 1 && ph7_value_is_numeric(apArg[0])) {
    ph7_result_int(pCtx, ph7_value_to_int(apArg[0]) & 0xFFFF);
  } else if (nArg == 2 && ph7_value_is_numeric(apArg[0]) && ph7_value_is_numeric(apArg[1])) {
    ph7_result_int(pCtx, ((ph7_value_to_int(apArg[0]) & 0xFF) << 8) | (ph7_value_to_int(apArg[1]) & 0xFF));
  } else {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "makeWord: expected 1 or 2 numeric arguments");
    ph7_result_null(pCtx);
  }
  return PH7_OK;
}

// randomSeed();
// No-op: we use HWRNG which requires no seeding
//
IMPLEMENT(randomSeed,
  /* no-op */
)

// useRealRandomGenerator();
// No-op: we use HWRNG always, no software PRNG fallback
//
IMPLEMENT(useRealRandomGenerator,
  /* no-op */
)

// Historically implemented via Impl; IMPLEMENT_N_N_N_N_N was not available by that time
// $scaled_x = map(long $x, long $in_min, long $in_max, long $out_min, long $out_max)
//
Impl(map, pCtx, nArg, apArg) {
  if (nArg != 5 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      !ph7_value_is_numeric(apArg[2]) ||
      !ph7_value_is_numeric(apArg[3]) ||
      !ph7_value_is_numeric(apArg[4])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "map: expected 5 numeric arguments");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const long x       = (long)ph7_value_to_int(apArg[0]);
  const long in_min  = (long)ph7_value_to_int(apArg[1]);
  const long in_max  = (long)ph7_value_to_int(apArg[2]);
  const long out_min = (long)ph7_value_to_int(apArg[3]);
  const long out_max = (long)ph7_value_to_int(apArg[4]);

  const long run = in_max - in_min;

  if (run == 0) {
    ph7_result_int(pCtx, -1);
    return PH7_OK;
  }

  const long rise  = out_max - out_min;
  const long delta = x - in_min;

  ph7_result_int(pCtx, (delta * rise) / run + out_min);
  return PH7_OK;
}


// Variable args Arduino function random is implemented via Impl as it gives more freedom
// in what to check and what input we can tolerate
// random($upper_limit)
// random($lower_limit, $upper_limit)
//
Impl(random, pCtx, nArg, apArg) {

  long min = 0, max = 0;

  if (nArg == 1 && ph7_value_is_numeric(apArg[0])) {
    max = (long)ph7_value_to_int(apArg[0]);
  } else if (nArg == 2 && ph7_value_is_numeric(apArg[0]) && ph7_value_is_numeric(apArg[1])) {
    min = (long)ph7_value_to_int(apArg[0]);
    max = (long)ph7_value_to_int(apArg[1]);
  } else {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "random: expected 1 or 2 numeric arguments");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  if (min >= max) {
    ph7_result_int(pCtx, min);
  } else {
#ifdef __CYGWIN__
    fprintf(stderr,"random() called");
#else
    ph7_result_int(pCtx, min + (long)(esp_random() % (unsigned long)(max - min)));
#endif
  }

  return PH7_OK;
}
/* These functions should be made optional: PHP has everything to work with characters and strings
 *
 * Arduino versions take an int code. PHP has no char type, so these accept ether
 * an int code or string (only first byte of the string is examined):
 *
 *   isDigit("7")  -> true          isDigit(7)   -> false (code 7 is BEL)
 *   toUpperCase("a") -> "A"        toUpperCase(97) -> 65
 *
 * Tests return bool. Converters return string if they got a string and int otherwise.
 */
static int ExtractChar(ph7_value *pArg, int *piChar, int *pbString) {

  if (ph7_value_is_string(pArg)) {

    int nLen = 0;
    const char *zStr = ph7_value_to_string(pArg, &nLen);

    if (zStr == NULL || nLen < 1)
      return 0;

    *piChar   = (unsigned char)zStr[0];
    *pbString = 1;

    return 1;
  }

  if (ph7_value_is_numeric(pArg)) {

    *piChar   = ph7_value_to_int(pArg);
    *pbString = 0;

    return 1;
  }
  return 0;
}

/* Templates
*/

/* 
 * expr_ is a C expression which uses iChar; it is evaluated for codes 0..255 only,
 * anything else is "not a member of the class"
 */
#define IMPLEMENT_CHAR_TEST(name, expr_)                                        \
  Impl(name, pCtx, nArg, apArg) {                                               \
    int iChar = 0, bString = 0;                                                 \
    if (nArg >= 1 && ExtractChar(apArg[0], &iChar, &bString)) {              \
      ph7_result_bool(pCtx, (iChar >= 0 && iChar <= 255) ? ((expr_) != 0) : 0); \
    } else {                                                                    \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                            \
        #name ": expected a character (string) or a character code (int)");     \
      ph7_result_null(pCtx);                                                    \
    }                                                                           \
    return PH7_OK;                                                              \
  }

/* expr_ is a C expression using `iChar`, evaluated for any int */
#define IMPLEMENT_CHAR_CONV(name, expr_)                                        \
  Impl(name, pCtx, nArg, apArg) {                                               \
    int iChar = 0, bString = 0;                                                 \
    if (nArg >= 1 && ExtractChar(apArg[0], &iChar, &bString)) {              \
      const int iRes = (expr_);                                                 \
      if (bString) {                                                            \
        const char cRes = (char)iRes;                                           \
        ph7_result_string(pCtx, &cRes, 1);                                      \
      } else {                                                                  \
        ph7_result_int(pCtx, iRes);                                             \
      }                                                                         \
    } else {                                                                    \
      ph7_context_throw_error(pCtx, PH7_CTX_WARNING,                            \
        #name ": expected a character (string) or a character code (int)");     \
      ph7_result_null(pCtx);                                                    \
    }                                                                           \
    return PH7_OK;                                                              \
  }

/* Implementation */
IMPLEMENT_CHAR_TEST(isAlpha,           isalpha(iChar))
IMPLEMENT_CHAR_TEST(isAlphaNumeric,    isalnum(iChar))
IMPLEMENT_CHAR_TEST(isAscii,           iChar <= 0x7F)
IMPLEMENT_CHAR_TEST(isControl,         iscntrl(iChar))
IMPLEMENT_CHAR_TEST(isDigit,           isdigit(iChar))
IMPLEMENT_CHAR_TEST(isGraph,           isgraph(iChar))
IMPLEMENT_CHAR_TEST(isHexadecimalDigit,isxdigit(iChar))
IMPLEMENT_CHAR_TEST(isLowerCase,       islower(iChar))
IMPLEMENT_CHAR_TEST(isPrintable,       isprint(iChar))
IMPLEMENT_CHAR_TEST(isPunct,           ispunct(iChar))
IMPLEMENT_CHAR_TEST(isSpace,           isspace(iChar))
IMPLEMENT_CHAR_TEST(isUpperCase,       isupper(iChar))
IMPLEMENT_CHAR_TEST(isWhitespace,      iChar == ' ' || iChar == '\t')
IMPLEMENT_CHAR_CONV(toAscii,           iChar & 0x7F)
IMPLEMENT_CHAR_CONV(toLowerCase,       (iChar >= 0 && iChar <= 255) ? tolower(iChar) : iChar)
IMPLEMENT_CHAR_CONV(toUpperCase,       (iChar >= 0 && iChar <= 255) ? toupper(iChar) : iChar)


/* Analog IO
 */

// $raw = analogRead(int $pin): int
//
IMPLEMENT_N(analogRead, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogRead(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, analogRead(a0));
#endif
)

// analogReadResolution(int $bits): void
//
IMPLEMENT_N(analogReadResolution, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogReadResolution(%d)\r\n",a0);
#else
  analogReadResolution(a0);
#endif
)

// analogSetWidth(int $bits): void
// Removed from Arduino Core 3.x, kept as an alias of analogReadResolution()
//
IMPLEMENT_N(analogSetWidth, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogSetWidth(%d)\r\n",a0);
#else
  analogReadResolution(a0);
#endif
)

// $mv = analogReadMilliVolts(int $pin): int
//
IMPLEMENT_N(analogReadMilliVolts, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogReadMilliVolts(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)analogReadMilliVolts(a0));
#endif
)

// analogWrite(int $pin, int $value): void
//
IMPLEMENT_N_N(analogWrite, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogWrite(%d, %d)\r\n",a0,a1);
#else
  analogWrite(a0, a1);
#endif
)

// analogWriteResolution(int $pin, int $bits): void
//
IMPLEMENT_N_N(analogWriteResolution, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogWriteResolution(%d, %d)\r\n",a0,a1);
#else
  analogWriteResolution(a0, a1);
#endif
)

// analogWriteFrequency(int $pin, int $freq_hz): void
//
IMPLEMENT_N_N(analogWriteFrequency, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"analogWriteFrequency(%d, %lld)\r\n",a0,a1);
#else
  analogWriteFrequency(a0, a1);
#endif
)

// analogSetAttenuation(int $attenuation): void       (ADC_0db, ADC_2_5db, ADC_6db, ADC_11db)
//
IMPLEMENT_N(analogSetAttenuation, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogSetAttenuation(%d)\r\n",a0);
#else
  analogSetAttenuation(a0);
#endif
)

// analogSetPinAttenuation(int $pin, int $attenuation): void
//
IMPLEMENT_N_N(analogSetPinAttenuation, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"analogSetPinAttenuation(%d, %d)\r\n",a0,a1);
#else
  analogSetPinAttenuation(a0, a1);
#endif
)


/*
 * LEDC (PWM)
 */

// bool ledcAttach(int $pin, int $freq_hz, int $resolution_bits)
//
IMPLEMENT_N_N_N(ledcAttach, int, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcAttach(%d, %lld, %d)\r\n",a0,a1,a2);
#else
  ph7_result_bool(pCtx, ledcAttach(a0, a1, a2));
#endif
)

// bool ledcAttachChannel(int $pin, int $freq_hz, int $resolution_bits, int $channel)
//
IMPLEMENT_N_N_N_N(ledcAttachChannel, int, long long, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcAttachChannel(%d, %lld, %d, %d)\r\n",a0,a1,a2,a3);
#else
  ph7_result_bool(pCtx, ledcAttachChannel(a0, a1, a2, a3));
#endif
)

// int $freq = ledcChangeFrequency(int $pin, int $freq_hz, int $resolution_bits)  (0 on error)
//
IMPLEMENT_N_N_N(ledcChangeFrequency, int, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcChangeFrequency(%d, %lld, %d)\r\n",a0,a1,a2);
#else
  ph7_result_int64(pCtx, (sxi64)ledcChangeFrequency(a0, a1, a2));
#endif
)

// bool ledcDetach(int $pin)
//
IMPLEMENT_N(ledcDetach, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcDetach(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, ledcDetach(a0));
#endif
)

// bool ledcFade(int $pin, int $start_duty, int $target_duty, int $max_fade_time_ms)
//
IMPLEMENT_N_N_N_N(ledcFade, int, long long, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcFade(%d, %lld, %lld, %d)\r\n",a0,a1,a2,a3);
#else
  ph7_result_bool(pCtx, ledcFade(a0, a1, a2, a3));
#endif
)

// bool ledcOutputInvert(int $pin, bool $invert)
//
IMPLEMENT_N_N(ledcOutputInvert, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcOutputInvert(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, ledcOutputInvert(a0, a1 ? 1 : 0));
#endif
)

// int $duty = ledcRead(int $pin)
//
IMPLEMENT_N(ledcRead, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcRead(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)ledcRead(a0));
#endif
)

// int $freq = ledcReadFreq(int $pin)
//
IMPLEMENT_N(ledcReadFreq, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcReadFreq(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)ledcReadFreq(a0));
#endif
)

// bool ledcWrite(int $pin, int $duty)
//
IMPLEMENT_N_N(ledcWrite, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcWrite(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, ledcWrite(a0, a1));
#endif
)

// bool ledcWriteChannel(int $channel, int $duty)
//
IMPLEMENT_N_N(ledcWriteChannel, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcWriteChannel(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, ledcWriteChannel(a0, a1));
#endif
)

// int $freq = ledcWriteNote(int $pin, int $note, int $octave)      ($note: NOTE_C .. NOTE_B = 0..11)
//
IMPLEMENT_N_N_N(ledcWriteNote, int, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcWriteNote(%d, %d, %d)\r\n",a0,a1,a2);
#else
  ph7_result_int64(pCtx, (sxi64)ledcWriteNote(a0, a1, a2));
#endif
)

// int $freq = ledcWriteTone(int $pin, int $freq_hz)         ($freq_hz = 0 stops the tone)
//
IMPLEMENT_N_N(ledcWriteTone, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"ledcWriteTone(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_int64(pCtx, (sxi64)ledcWriteTone(a0, a1));
#endif
)


/*
 * CPU, clocks, internal temperature sensor
 */

// $hz = getApbFrequency(): int
//
IMPLEMENT(getApbFrequency,
#ifdef __CYGWIN__
  fprintf(stderr,"getApbFrequency()\r\n");
#else
  ph7_result_int64(pCtx, (sxi64)getApbFrequency());
#endif
)

// $mhz = getCpuFrequencyMhz(): int
//
IMPLEMENT(getCpuFrequencyMhz,
#ifdef __CYGWIN__
  fprintf(stderr,"getCpuFrequencyMhz()\r\n");
#else
  ph7_result_int64(pCtx, (sxi64)getCpuFrequencyMhz());
#endif
)

// $mhz = getXtalFrequencyMhz(): int
//
IMPLEMENT(getXtalFrequencyMhz,
#ifdef __CYGWIN__
  fprintf(stderr,"getXtalFrequencyMhz()\r\n");
#else
  ph7_result_int64(pCtx, (sxi64)getXtalFrequencyMhz());
#endif
)

// bool setCpuFrequencyMhz(int $mhz)
//
IMPLEMENT_N(setCpuFrequencyMhz, int,
#ifdef __CYGWIN__
  fprintf(stderr,"setCpuFrequencyMhz(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, setCpuFrequencyMhz(a0));
#endif
)

// float $celsius = temperatureRead()
//
IMPLEMENT(temperatureRead,
#ifdef __CYGWIN__
  fprintf(stderr,"temperatureRead()\r\n");
#else
  ph7_result_double(pCtx, (double)temperatureRead());
#endif
)


/*
 * Touch sensor (where available)
 */

// int $value = touchRead(int $pin)
//
IMPLEMENT_N(touchRead, int,
#ifdef __CYGWIN__
  fprintf(stderr,"touchRead(%d)\r\n",a0);
#else
#  ifdef SOC_TOUCH_SENSOR_SUPPORTED
  ph7_result_int64(pCtx, (sxi64)touchRead(a0));
#  else
  NOT_SUPPORTED(touchRead);
#  endif
#endif
)


/* 
 * Sigma-Delta modulator
 */

// bool sigmaDeltaAttach(int $pin, int $freq_hz)
//
IMPLEMENT_N_N(sigmaDeltaAttach, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"sigmaDeltaAttach(%d, %lld)\r\n",a0,a1);
#else
#  ifdef SOC_SDM_SUPPORTED
  ph7_result_bool(pCtx, sigmaDeltaAttach(a0, a1));
#  else
  NOT_SUPPORTED(sigmaDeltaAttach);
#  endif
#endif
)

// bool sigmaDeltaWrite(int $pin, int $duty)
//
IMPLEMENT_N_N(sigmaDeltaWrite, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"sigmaDeltaWrite(%d, %d)\r\n",a0,a1);
#else
#ifdef SOC_SDM_SUPPORTED
  ph7_result_bool(pCtx, sigmaDeltaWrite(a0, a1));
#else
  NOT_SUPPORTED(sigmaDeltaWrite);
#endif
#endif
)

// bool sigmaDeltaDetach(int $pin)
//
IMPLEMENT_N(sigmaDeltaDetach, int,
#ifdef __CYGWIN__
  fprintf(stderr,"sigmaDeltaDetach(%d)\r\n",a0);
#else
#ifdef SOC_SDM_SUPPORTED
  ph7_result_bool(pCtx, sigmaDeltaDetach(a0));
#else
  NOT_SUPPORTED(sigmaDeltaDetach);
#endif
#endif
)

/*
 * I2C
 *
 *   i2cRead()              returns string of bytes read, or false on error
 *   i2cWriteReadNonStop()  same
 *   i2cGetClock()          returns frequency, or false on error
 *
 * Functions which have nothing to return except the status (i2cInit, i2cWrite etc)
 * return esp_err_t as is: 0 is ESP_OK, anything else is an error code.
 */

#define ESP32_I2C_TIMEOUT_MS   1000     /* default timeout for i2cWrite() and friends */
#define ESP32_IO_BUFFER_MAX    65536    /* sanity limit for the size of a single read */

// int $err = i2cInit(int $bus, int $sda, int $scl, int $freq_hz)
//
IMPLEMENT_N_N_N_N(i2cInit, int, int, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"i2cInit(%d, %d, %d, %lld)\r\n",a0,a1,a2,a3);
#else
  ph7_result_int(pCtx, (int)i2cInit(a0, a1, a2, a3));
#endif
)

// int $err = i2cDeinit(int $bus)
//
IMPLEMENT_N(i2cDeinit, int,
#ifdef __CYGWIN__
  fprintf(stderr,"i2cDeinit(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, (int)i2cDeinit(a0));
#endif
)

// bool i2cIsInit(int $bus)
//
IMPLEMENT_N(i2cIsInit, int,
#ifdef __CYGWIN__
  fprintf(stderr,"i2cIsInit(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, i2cIsInit(a0));
#endif
)

// int $err = i2cSetClock(int $bus, int $freq_hz)
//
IMPLEMENT_N_N(i2cSetClock, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"i2cSetClock(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_int(pCtx, (int)i2cSetClock(a0, a1));
#endif
)

// int|false $freq_hz = i2cGetClock(int $bus)
//
IMPLEMENT_N(i2cGetClock, int,
#ifdef __CYGWIN__
  fprintf(stderr,"i2cGetClock(%d)\r\n",a0);
#else
  uint32_t nFreq = 0;
  if (i2cGetClock(a0, &nFreq) == ESP_OK)
    ph7_result_int64(pCtx, (sxi64)nFreq);
  else
    ph7_result_bool(pCtx, 0);
#endif
)

// int $err = i2cWrite(int $bus, int $address, string $data [, int $timeout_ms = 1000])
//
Impl(i2cWrite, pCtx, nArg, apArg) {

  if (nArg < 3 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      !ph7_value_is_string(apArg[2])  ||
      (nArg >= 4 && !ph7_value_is_numeric(apArg[3]))) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "i2cWrite: expected (int $bus, int $address, string $data [, int $timeout_ms])");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iBus     = ph7_value_to_int(apArg[0]);
  const int   iAddress = ph7_value_to_int(apArg[1]);
  int         nLen     = 0;
  const char *zData    = ph7_value_to_string(apArg[2], &nLen);
  const sxi64 nTimeout = (nArg >= 4) ? ph7_value_to_int64(apArg[3]) : ESP32_I2C_TIMEOUT_MS;

 

#ifdef __CYGWIN__
  fprintf(stderr,"i2cWrite(%d, 0x%02x, <%d bytes>, %lld)\r\n", iBus, iAddress, nLen, (long long)nTimeout);
#else
  ph7_result_int(pCtx, (int)i2cWrite(iBus, iAddress, (const uint8_t *)zData, (size_t)nLen, (uint32_t)nTimeout));
#endif
  return PH7_OK;
}

// string|false $data = i2cRead(int $bus, int $address, int $size [, int $timeout_ms = 1000])
//
Impl(i2cRead, pCtx, nArg, apArg) {

  if (nArg < 3 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      !ph7_value_is_numeric(apArg[2]) ||
      (nArg >= 4 && !ph7_value_is_numeric(apArg[3]))) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "i2cRead: expected (int $bus, int $address, int $size [, int $timeout_ms])");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iBus     = ph7_value_to_int(apArg[0]);
  const int   iAddress = ph7_value_to_int(apArg[1]);
  const int   nSize    = ph7_value_to_int(apArg[2]);
  const sxi64 nTimeout = (nArg >= 4) ? ph7_value_to_int64(apArg[3]) : ESP32_I2C_TIMEOUT_MS;

  if (nSize < 1 || nSize > ESP32_IO_BUFFER_MAX) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "i2cRead: invalid $size");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

 

#ifdef __CYGWIN__
  fprintf(stderr,"i2cRead(%d, 0x%02x, %d, %lld)\r\n", iBus, iAddress, nSize, (long long)nTimeout);
#else
  uint8_t *pBuf = (uint8_t *)malloc((size_t)nSize);
  if (pBuf == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "i2cRead: out of memory");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  size_t nRead = 0;
  if (i2cRead(iBus, iAddress, pBuf, (size_t)nSize, (uint32_t)nTimeout, &nRead) == ESP_OK)
    ph7_result_string(pCtx, (const char *)pBuf, (int)nRead);
  else
    ph7_result_bool(pCtx, 0);
  free(pBuf);
#endif
  return PH7_OK;
}

// string|false $data = i2cWriteReadNonStop(int $bus, int $address, string $data, int $read_size [, int $timeout_ms = 1000])
// Write, then read after a repeated START (no STOP in between): register reads of most I2C sensors
//
Impl(i2cWriteReadNonStop, pCtx, nArg, apArg) {

  if (nArg < 4 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      !ph7_value_is_string(apArg[2])  ||
      !ph7_value_is_numeric(apArg[3]) ||
      (nArg >= 5 && !ph7_value_is_numeric(apArg[4]))) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "i2cWriteReadNonStop: expected (int $bus, int $address, string $data, int $read_size [, int $timeout_ms])");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iBus     = ph7_value_to_int(apArg[0]);
  const int   iAddress = ph7_value_to_int(apArg[1]);
  int         nLen     = 0;
  const char *zData    = ph7_value_to_string(apArg[2], &nLen);
  const int   nSize    = ph7_value_to_int(apArg[3]);
  const sxi64 nTimeout = (nArg >= 5) ? ph7_value_to_int64(apArg[4]) : ESP32_I2C_TIMEOUT_MS;

  if (nSize < 1 || nSize > ESP32_IO_BUFFER_MAX) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "i2cWriteReadNonStop: invalid $read_size");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"i2cWriteReadNonStop(%d, 0x%02x, <%d bytes>, %d, %lld)\r\n", iBus, iAddress, nLen, nSize, (long long)nTimeout);
#else
  uint8_t *pBuf = (uint8_t *)malloc((size_t)nSize);
  if (pBuf == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "i2cWriteReadNonStop: out of memory");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  uint32_t nRead = 0;
  if (i2cWriteReadNonStop(iBus, iAddress, (const uint8_t *)zData, (size_t)nLen, pBuf, (size_t)nSize, (uint32_t)nTimeout, &nRead) == ESP_OK)
    ph7_result_string(pCtx, (const char *)pBuf, (int)nRead);
  else
    ph7_result_bool(pCtx, 0);
  free(pBuf);
#endif
  return PH7_OK;
}


/*
 * Hardware timers
 *
 *   $t = timerBegin(1000000);         // 1 MHz tick, returns handle or false
 *   timerAlarm($t, 500000, true, 0);  // alarm every 0.5 s
 *   timerEnd($t);                     // frees the handle (and the hardware timer)
 *
 * A handle is an index into the global table `apTimer`: see "Handle tables" above.
 */

// int|false $timer = timerBegin(int $freq_hz)
// Returns timer handle (an index) or false if there are no free timers / the core refused
//
Impl(timerBegin, pCtx, nArg, apArg) {

  if (nArg < 1 || !ph7_value_is_numeric(apArg[0])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerBegin: expected (int $freq_hz)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const sxi64 nFreq = ph7_value_to_int64(apArg[0]);
  const int   iSlot = ht_reserve(apTimer, ESP32_TIMER_MAX);

  if (iSlot < 0) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerBegin: no free hardware timers");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  void *pTimer;
  (void)nFreq;

#ifdef __CYGWIN__
  fprintf(stderr,"timerBegin(%lld) = handle %d\r\n", (long long)nFreq, iSlot);
  pTimer = HT_FAKE;
#else
  pTimer = (void *)timerBegin((uint32_t)nFreq);
#endif

  ht_commit(apTimer, iSlot, pTimer);   /* NULL frees the slot we have reserved */

  if (pTimer != NULL) {
    ph7_result_int(pCtx, iSlot);
  } else {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerBegin: failed");
    ph7_result_bool(pCtx, 0);
  }
  return PH7_OK;
}

// timerEnd(int $timer): void
// Stops the timer, frees the hardware and the handle: $timer becomes invalid
//
Impl(timerEnd, pCtx, nArg, apArg) {

  int   iIdx   = -1;
  void *pTimer = NULL;

  if (nArg >= 1 && ph7_value_is_numeric(apArg[0])) {
    iIdx   = ph7_value_to_int(apArg[0]);
    pTimer = ht_get_for_close(apTimer, ESP32_TIMER_MAX, iIdx);
  }

  if (pTimer == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerEnd: invalid handle");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"timerEnd(%d)\r\n", iIdx);
#else
  timerEnd((hw_timer_t *)pTimer);
#endif

  ht_commit(apTimer, iIdx, NULL);
  return PH7_OK;
}

// timerStart(int $timer): void
//
IMPLEMENT_H(timerStart, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerStart(%d)\r\n",a0);
#else
  timerStart(pHandle);
#endif
)

// timerStop(int $timer): void
//
IMPLEMENT_H(timerStop, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerStop(%d)\r\n",a0);
#else
  timerStop(pHandle);
#endif
)

// timerRestart(int $timer): void
//
IMPLEMENT_H(timerRestart, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerRestart(%d)\r\n",a0);
#else
  timerRestart(pHandle);
#endif
)

// timerWrite(int $timer, int $value): void
// Set the counter
//
IMPLEMENT_H_N(timerWrite, hw_timer_t, esp32_timer_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"timerWrite(%d, %lld)\r\n",a0,a1);
#else
  timerWrite(pHandle, (uint64_t)a1);
#endif
)

// timerAlarm(int $timer, int $alarm_value, bool $autoreload, int $reload_count): void
// $alarm_value is in timer ticks. $reload_count: counter value to restart from
//
IMPLEMENT_H_N_N_N(timerAlarm, hw_timer_t, esp32_timer_get, long long, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"timerAlarm(%d, %lld, %d, %lld)\r\n",a0,a1,a2,a3);
#else
  timerAlarm(pHandle, (uint64_t)a1, a2 != 0, (uint64_t)a3);
#endif
)

// int $ticks = timerRead(int $timer)
//
IMPLEMENT_H(timerRead, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerRead(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)timerRead(pHandle));
#endif
)

// int $us = timerReadMicros(int $timer)
//
IMPLEMENT_H(timerReadMicros, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerReadMicros(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)timerReadMicros(pHandle));
#endif
)

// int $ms = timerReadMillis(int $timer)
//
IMPLEMENT_H(timerReadMillis, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerReadMillis(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)timerReadMillis(pHandle));
#endif
)

// float $sec = timerReadSeconds(int $timer)
//
IMPLEMENT_H(timerReadSeconds, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerReadSeconds(%d)\r\n",a0);
#else
  ph7_result_double(pCtx, (double)timerReadSeconds(pHandle));
#endif
)

// int $hz = timerGetFrequency(int $timer)
//
IMPLEMENT_H(timerGetFrequency, hw_timer_t, esp32_timer_get,
#ifdef __CYGWIN__
  fprintf(stderr,"timerGetFrequency(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)timerGetFrequency(pHandle));
#endif
)


/*
 * UART
 *
 * The handle IS the UART number: uartBegin(1, 115200) returns 1, and every other
 * function takes that number.
 *
 */

/* uartBegin() defaults. These are the values HardwareSerial uses. Serial config is
 * SERIAL_8N1 (HardwareSerial.h is C++ only, so the value is repeated here)
 */
#define ESP32_UART_CONFIG_8N1   0x800001c
#define ESP32_UART_RX_BUFFER    256
#define ESP32_UART_TX_BUFFER    0
#define ESP32_UART_FIFO_FULL    120
#define ESP32_UART_TIMEOUT_MS   1000

// int|false $uart = uartBegin(int $uart_nr, int $baud [, int $config = SERIAL_8N1, int $rx_pin = -1, int $tx_pin = -1, int $rx_buffer = 256, int $tx_buffer = 0, bool $inverted = false, int $rxfifo_full = 120])
// Pin -1 means "default pin". Returns the handle (== $uart_nr) or false.
//
Impl(uartBegin, pCtx, nArg, apArg) {

  int i, bBad = (nArg < 2);

  for (i = 0; i < nArg && i < 9; i++)
    if (!ph7_value_is_numeric(apArg[i]))
      bBad = 1;

  if (bBad) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING,
      "uartBegin: expected (int $uart_nr, int $baud [, int $config, int $rx_pin, int $tx_pin, int $rx_buffer, int $tx_buffer, bool $inverted, int $rxfifo_full])");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iNum      = ph7_value_to_int(apArg[0]);
  const sxi64 nBaud     = ph7_value_to_int64(apArg[1]);
  const sxi64 nConfig   = (nArg > 2) ? ph7_value_to_int64(apArg[2]) : ESP32_UART_CONFIG_8N1;
  const int   iRxPin    = (nArg > 3) ? ph7_value_to_int(apArg[3])   : -1;
  const int   iTxPin    = (nArg > 4) ? ph7_value_to_int(apArg[4])   : -1;
  const sxi64 nRxBuffer = (nArg > 5) ? ph7_value_to_int64(apArg[5]) : ESP32_UART_RX_BUFFER;
  const sxi64 nTxBuffer = (nArg > 6) ? ph7_value_to_int64(apArg[6]) : ESP32_UART_TX_BUFFER;
  const int   bInverted = (nArg > 7) ? (ph7_value_to_int(apArg[7]) != 0) : 0;
  const int   iFifoFull = (nArg > 8) ? ph7_value_to_int(apArg[8])   : ESP32_UART_FIFO_FULL;

  if (iNum < 0 || iNum >= ESP32_UART_MAX || nBaud < 1 || nRxBuffer < 0 || nTxBuffer < 0) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartBegin: bad UART number, baud rate or buffer size");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  void *pPrev = NULL;
  void *pUart = NULL;

  if (!ht_open(apUart, ESP32_UART_MAX, iNum, &pPrev)) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartBegin: this UART is being opened or closed by someone else");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"uartBegin(%d, %lld, 0x%llx, %d, %d, %lld, %lld, %d, %d) = handle %d\r\n", iNum, (long long)nBaud, (long long)nConfig,
          iRxPin, iTxPin, (long long)nRxBuffer, (long long)nTxBuffer, bInverted, iFifoFull, iNum);
  pUart = HT_FAKE;
#else
  pUart = (void *)uartBegin((uint8_t)iNum, (uint32_t)nBaud, (uint32_t)nConfig, (int8_t)iRxPin, (int8_t)iTxPin,
                            (uint32_t)nRxBuffer, (uint32_t)nTxBuffer, bInverted != 0, (uint8_t)iFifoFull);
#endif

  /* If the core failed, an already open UART keeps its old handle */
  ht_commit(apUart, iNum, (pUart != NULL) ? pUart : pPrev);

  if (pUart != NULL) {
    ph7_result_int(pCtx, iNum);
  } else {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartBegin: failed");
    ph7_result_bool(pCtx, 0);
  }
  return PH7_OK;
}

// uartEnd(int $uart): void
// Uninstalls the driver, the handle becomes invalid
//
Impl(uartEnd, pCtx, nArg, apArg) {

  int   iIdx  = -1;
  void *pUart = NULL;

  if (nArg >= 1 && ph7_value_is_numeric(apArg[0])) {
    iIdx  = ph7_value_to_int(apArg[0]);
    pUart = ht_get_for_close(apUart, ESP32_UART_MAX, iIdx);
  }

  if (pUart == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartEnd: invalid handle");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"uartEnd(%d)\r\n", iIdx);
#else
  uartEnd((uint8_t)iIdx);
#endif

  ht_commit(apUart, iIdx, NULL);
  return PH7_OK;
}

// bool uartIsDriverInstalled(int $uart)
// A UART which was never opened through uartBegin() is simply "not installed"
//
Impl(uartIsDriverInstalled, pCtx, nArg, apArg) {

  if (nArg < 1 || !ph7_value_is_numeric(apArg[0])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartIsDriverInstalled: expected (int $uart)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int iIdx = ph7_value_to_int(apArg[0]);
  uart_t   *pUart = esp32_uart_get(iIdx);

#ifdef __CYGWIN__
  fprintf(stderr,"uartIsDriverInstalled(%d)\r\n", iIdx);
  ph7_result_bool(pCtx, pUart != NULL);
#else
  ph7_result_bool(pCtx, pUart != NULL && uartIsDriverInstalled(pUart));
#endif
  return PH7_OK;
}

// int $count = uartAvailable(int $uart)
//
IMPLEMENT_H(uartAvailable, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartAvailable(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)uartAvailable(pHandle));
#endif
)

// int $count = uartAvailableForWrite(int $uart)
//
IMPLEMENT_H(uartAvailableForWrite, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartAvailableForWrite(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)uartAvailableForWrite(pHandle));
#endif
)

// int $byte = uartRead(int $uart)       -1 if there is nothing to read
//
IMPLEMENT_H(uartRead, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartRead(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, uartAvailable(pHandle) ? (int)uartRead(pHandle) : -1);
#endif
)

// int $byte = uartPeek(int $uart)       -1 if there is nothing to read
//
IMPLEMENT_H(uartPeek, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartPeek(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, uartAvailable(pHandle) ? (int)uartPeek(pHandle) : -1);
#endif
)

// string $data = uartReadBytes(int $uart, int $size [, int $timeout_ms = 1000])
// Reads up to $size bytes, waits up to $timeout_ms for them.
//
Impl(uartReadBytes, pCtx, nArg, apArg) {

  uart_t *pUart = NULL;

  if (nArg >= 2 &&
      ph7_value_is_numeric(apArg[0]) &&
      ph7_value_is_numeric(apArg[1]) &&
      (nArg < 3 || ph7_value_is_numeric(apArg[2])))
    pUart = esp32_uart_get(ph7_value_to_int(apArg[0]));

  if (pUart == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartReadBytes: expected (int $uart, int $size [, int $timeout_ms]) and a valid handle");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iIdx     = ph7_value_to_int(apArg[0]);
  const int   nSize    = ph7_value_to_int(apArg[1]);
  const sxi64 nTimeout = (nArg >= 3) ? ph7_value_to_int64(apArg[2]) : ESP32_UART_TIMEOUT_MS;

  if (nSize < 1 || nSize > ESP32_IO_BUFFER_MAX) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartReadBytes: invalid $size");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"uartReadBytes(%d, %d, %lld)\r\n", iIdx, nSize, (long long)nTimeout);
#else
  uint8_t *pBuf = (uint8_t *)malloc((size_t)nSize);
  if (pBuf == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "uartReadBytes: out of memory");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  const size_t nRead = uartReadBytes(pUart, pBuf, (size_t)nSize, (uint32_t)(nTimeout < 0 ? 0 : nTimeout));
  ph7_result_string(pCtx, (const char *)pBuf, (int)nRead);
  free(pBuf);
#endif
  return PH7_OK;
}

// uartWrite(int $uart, int $byte): void
//
IMPLEMENT_H_N(uartWrite, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartWrite(%d, %d)\r\n",a0,a1);
#else
  uartWrite(pHandle, (uint8_t)a1);
#endif
)

// uartWriteBuf(int $uart, string $data): void
//
IMPLEMENT_H_S(uartWriteBuf, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartWriteBuf(%d, <%d bytes>)\r\n",a0,nLen);
#else
  uartWriteBuf(pHandle, (const uint8_t *)zData, (size_t)nLen);
#endif
)

// uartFlush(int $uart): void
//
IMPLEMENT_H(uartFlush, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartFlush(%d)\r\n",a0);
#else
  uartFlush(pHandle);
#endif
)

// uartFlushTxOnly(int $uart, bool $tx_only): void
//
IMPLEMENT_H_N(uartFlushTxOnly, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartFlushTxOnly(%d, %d)\r\n",a0,a1);
#else
  uartFlushTxOnly(pHandle, a1 != 0);
#endif
)

// bool uartSetBaudRate(int $uart, int $baud)
//
IMPLEMENT_H_N(uartSetBaudRate, uart_t, esp32_uart_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetBaudRate(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetBaudRate(pHandle, (uint32_t)a1));
#endif
)

// int $baud = uartGetBaudRate(int $uart)
//
IMPLEMENT_H(uartGetBaudRate, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartGetBaudRate(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)uartGetBaudRate(pHandle));
#endif
)

// bool uartPinSignalInversion(int $uart, int $inv_mask, bool $inverted)
// $inv_mask: UART_SIGNAL_RXD_INV, UART_SIGNAL_TXD_INV, UART_SIGNAL_RTS_INV, UART_SIGNAL_CTS_INV
//
IMPLEMENT_H_N_N(uartPinSignalInversion, uart_t, esp32_uart_get, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartPinSignalInversion(%d, 0x%llx, %d)\r\n",a0,a1,a2);
#else
  ph7_result_bool(pCtx, uartPinSignalInversion(pHandle, (uint32_t)a1, a2 != 0));
#endif
)

// bool uartSetRxInvert(int $uart, bool $invert)
//
IMPLEMENT_H_N(uartSetRxInvert, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetRxInvert(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetRxInvert(pHandle, a1 != 0));
#endif
)

// bool uartSetTxInvert(int $uart, bool $invert)
//
IMPLEMENT_H_N(uartSetTxInvert, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetTxInvert(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetTxInvert(pHandle, a1 != 0));
#endif
)

// bool uartSetCtsInvert(int $uart, bool $invert)
//
IMPLEMENT_H_N(uartSetCtsInvert, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetCtsInvert(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetCtsInvert(pHandle, a1 != 0));
#endif
)

// bool uartSetRtsInvert(int $uart, bool $invert)
//
IMPLEMENT_H_N(uartSetRtsInvert, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetRtsInvert(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetRtsInvert(pHandle, a1 != 0));
#endif
)

// bool uartSetRxTimeout(int $uart, int $symbols)
//
IMPLEMENT_H_N(uartSetRxTimeout, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetRxTimeout(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetRxTimeout(pHandle, (uint8_t)a1));
#endif
)

// bool uartSetRxFIFOFull(int $uart, int $bytes)
//
IMPLEMENT_H_N(uartSetRxFIFOFull, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetRxFIFOFull(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetRxFIFOFull(pHandle, (uint8_t)a1));
#endif
)

// uartSetFastReading(int $uart): void
//
IMPLEMENT_H(uartSetFastReading, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetFastReading(%d)\r\n",a0);
#else
  uartSetFastReading(pHandle);
#endif
)

// uartSetDebug(int $uart): void
//
Impl(uartSetDebug, pCtx, nArg, apArg) {

  if (nArg < 1 || !ph7_value_is_numeric(apArg[0])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartSetDebug: expected (int $uart), -1 to disable");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int iIdx  = ph7_value_to_int(apArg[0]);
  uart_t   *pUart = (iIdx < 0) ? NULL : esp32_uart_get(iIdx);

  if (iIdx >= 0 && pUart == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "uartSetDebug: invalid handle");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"uartSetDebug(%d)\r\n", iIdx);
#else
  uartSetDebug(pUart);
#endif
  return PH7_OK;
}

// int $uart = uartGetDebug()       UART number which is used for debug output, -1 if none
//
IMPLEMENT(uartGetDebug,
#ifdef __CYGWIN__
  fprintf(stderr,"uartGetDebug()\r\n");
#else
  ph7_result_int(pCtx, uartGetDebug());
#endif
)

// bool uartSetPins(int $uart_nr, int $rx, int $tx, int $cts, int $rts)
//
IMPLEMENT_N_N_N_N_N(uartSetPins, int, int, int, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetPins(%d, %d, %d, %d, %d)\r\n",a0,a1,a2,a3,a4);
#else
  ph7_result_bool(pCtx, uartSetPins(a0, a1, a2, a3, a4));
#endif
)

// bool uartSetHwFlowCtrlMode(int $uart, int $mode, int $threshold)
//
IMPLEMENT_H_N_N(uartSetHwFlowCtrlMode, uart_t, esp32_uart_get, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetHwFlowCtrlMode(%d, %d, %d)\r\n",a0,a1,a2);
#else
  ph7_result_bool(pCtx, uartSetHwFlowCtrlMode(pHandle, a1, (uint8_t)a2));
#endif
)

// bool uartSetMode(int $uart, int $mode)
//
IMPLEMENT_H_N(uartSetMode, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetMode(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetMode(pHandle, a1));
#endif
)

// bool uartSetIrdaDirection(int $uart, int $direction)       ESP32_UART_IRDA_RX 0, ESP32_UART_IRDA_TX 1
// Only after uartSetMode($uart, UART_MODE_IRDA)
//
IMPLEMENT_H_N(uartSetIrdaDirection, uart_t, esp32_uart_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetIrdaDirection(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetIrdaDirection(pHandle, a1));
#endif
)

// bool uartSetClockSource(int $uart_nr, int $clock_source)
// Must be called BEFORE uartBegin(), so it takes the UART number, not a handle
//
IMPLEMENT_N_N(uartSetClockSource, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartSetClockSource(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartSetClockSource(a0, a1));
#endif
)

// bool uartEnableRxInternalPull(int $uart_nr, bool $enable)
// Must be called BEFORE uartBegin(), so it takes the UART number, not a handle
//
IMPLEMENT_N_N(uartEnableRxInternalPull, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"uartEnableRxInternalPull(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, uartEnableRxInternalPull(a0, a1 != 0));
#endif
)

// uartStartDetectBaudrate(int $uart): void
//
IMPLEMENT_H(uartStartDetectBaudrate, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartStartDetectBaudrate(%d)\r\n",a0);
#else
  uartStartDetectBaudrate(pHandle);
#endif
)

// int $baud = uartDetectBaudrate(int $uart)
// Blocking, see uartStartDetectBaudrate()
//
IMPLEMENT_H(uartDetectBaudrate, uart_t, esp32_uart_get,
#ifdef __CYGWIN__
  fprintf(stderr,"uartDetectBaudrate(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)uartDetectBaudrate(pHandle));
#endif
)


/* 
 * SPI, low level HAL
 *
 * The handle IS the bus number (FSPI/HSPI/VSPI, they differ between chips
 *   $spi = spiStartBus(HSPI, SPI_CLOCK_DIV2, SPI_MODE0, SPI_MSBFIRST);
 *   spiAttachSCK($spi, 14);
 *   spiAttachMISO($spi, 12);
 *   spiAttachMOSI($spi, 13);
 *   $in = spiTransferBytes($spi, "\x9f\x00\x00\x00");
 *
 * The *NL ("no lock") functions must be called between spiTransaction() or
 * spiSimpleTransaction() and spiEndTransaction(). A script which dies in between leaves
 * the bus locked, there is nothing here which would unlock it. Boom.
 */

// int|false $spi = spiStartBus(int $spi_num, int $clock_div, int $data_mode, int $bit_order)
//
Impl(spiStartBus, pCtx, nArg, apArg) {

  if (nArg < 4 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_numeric(apArg[1]) ||
      !ph7_value_is_numeric(apArg[2]) ||
      !ph7_value_is_numeric(apArg[3])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiStartBus: expected (int $spi_num, int $clock_div, int $data_mode, int $bit_order)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iNum     = ph7_value_to_int(apArg[0]);
  const sxi64 nClkDiv  = ph7_value_to_int64(apArg[1]);
  const int   iMode    = ph7_value_to_int(apArg[2]);
  const int   iOrder   = ph7_value_to_int(apArg[3]);
  void       *pPrev    = NULL;
  void       *pSpi     = NULL;

  if (!ht_open(apSpi, ESP32_SPI_MAX, iNum, &pPrev)) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiStartBus: bad bus number, or the bus is being opened or closed by someone else");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"spiStartBus(%d, 0x%llx, %d, %d) = handle %d\r\n", iNum, (long long)nClkDiv, iMode, iOrder, iNum);
  pSpi = HT_FAKE;
#else
  pSpi = (void *)spiStartBus((uint8_t)iNum, (uint32_t)nClkDiv, (uint8_t)iMode, (uint8_t)iOrder);
#endif

  /* If the core refused, an already started bus keeps its old handle */
  ht_commit(apSpi, iNum, (pSpi != NULL) ? pSpi : pPrev);

  if (pSpi != NULL) {
    ph7_result_int(pCtx, iNum);
  } else {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiStartBus: failed");
    ph7_result_bool(pCtx, 0);
  }
  return PH7_OK;
}

// spiStopBus(int $spi): void
// Stops the bus, the handle becomes invalid
//
Impl(spiStopBus, pCtx, nArg, apArg) {

  int   iIdx = -1;
  void *pSpi = NULL;

  if (nArg >= 1 && ph7_value_is_numeric(apArg[0])) {
    iIdx = ph7_value_to_int(apArg[0]);
    pSpi = ht_get_for_close(apSpi, ESP32_SPI_MAX, iIdx);
  }

  if (pSpi == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiStopBus: invalid handle");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

#ifdef __CYGWIN__
  fprintf(stderr,"spiStopBus(%d)\r\n", iIdx);
#else
  spiStopBus((spi_t *)pSpi);
#endif

  ht_commit(apSpi, iIdx, NULL);
  return PH7_OK;
}

// bool spiAttachSCK(int $spi, int $pin)
//
IMPLEMENT_H_N(spiAttachSCK, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiAttachSCK(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, spiAttachSCK(pHandle, (int8_t)a1));
#endif
)

// bool spiAttachMISO(int $spi, int $pin)
//
IMPLEMENT_H_N(spiAttachMISO, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiAttachMISO(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, spiAttachMISO(pHandle, (int8_t)a1));
#endif
)

// bool spiAttachMOSI(int $spi, int $pin)
//
IMPLEMENT_H_N(spiAttachMOSI, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiAttachMOSI(%d, %d)\r\n",a0,a1);
#else
  ph7_result_bool(pCtx, spiAttachMOSI(pHandle, (int8_t)a1));
#endif
)

// bool spiDetachSCK(int $spi)
//
IMPLEMENT_H(spiDetachSCK, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiDetachSCK(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, spiDetachSCK(pHandle));
#endif
)

// bool spiDetachMISO(int $spi)
//
IMPLEMENT_H(spiDetachMISO, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiDetachMISO(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, spiDetachMISO(pHandle));
#endif
)

// bool spiDetachMOSI(int $spi)
//
IMPLEMENT_H(spiDetachMOSI, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiDetachMOSI(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, spiDetachMOSI(pHandle));
#endif
)

// bool spiAttachSS(int $spi, int $ss_num, int $pin)      $ss_num: SPI_SS0 .. SPI_SS2
//
IMPLEMENT_H_N_N(spiAttachSS, spi_t, esp32_spi_get, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiAttachSS(%d, %d, %d)\r\n",a0,a1,a2);
#else
  ph7_result_bool(pCtx, spiAttachSS(pHandle, (uint8_t)a1, (int8_t)a2));
#endif
)

// bool spiDetachSS(int $spi)
//
IMPLEMENT_H(spiDetachSS, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiDetachSS(%d)\r\n",a0);
#else
  ph7_result_bool(pCtx, spiDetachSS(pHandle));
#endif
)

// spiEnableSSPins(int $spi, int $ss_mask): void           $ss_mask: SPI_SS_MASK_ALL = 7
//
IMPLEMENT_H_N(spiEnableSSPins, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiEnableSSPins(%d, %d)\r\n",a0,a1);
#else
  spiEnableSSPins(pHandle, (uint8_t)a1);
#endif
)

// spiDisableSSPins(int $spi, int $ss_mask): void
//
IMPLEMENT_H_N(spiDisableSSPins, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiDisableSSPins(%d, %d)\r\n",a0,a1);
#else
  spiDisableSSPins(pHandle, (uint8_t)a1);
#endif
)

// spiSSEnable(int $spi): void        hardware control of SS
//
IMPLEMENT_H(spiSSEnable, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSSEnable(%d)\r\n",a0);
#else
  spiSSEnable(pHandle);
#endif
)

// spiSSDisable(int $spi): void
//
IMPLEMENT_H(spiSSDisable, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSSDisable(%d)\r\n",a0);
#else
  spiSSDisable(pHandle);
#endif
)

// spiSSSet(int $spi): void           activate enabled SS pins
//
IMPLEMENT_H(spiSSSet, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSSSet(%d)\r\n",a0);
#else
  spiSSSet(pHandle);
#endif
)

// spiSSClear(int $spi): void         deactivate enabled SS pins
//
IMPLEMENT_H(spiSSClear, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSSClear(%d)\r\n",a0);
#else
  spiSSClear(pHandle);
#endif
)

// spiWaitReady(int $spi): void
//
IMPLEMENT_H(spiWaitReady, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWaitReady(%d)\r\n",a0);
#else
  spiWaitReady(pHandle);
#endif
)

// spiSSInvert(int $spi, bool $invert): void
IMPLEMENT_H_N(spiSSInvert, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSSInvert(%d, %d)\r\n",a0,a1);
#else
  spiSSInvert(pHandle, a1 != 0);
#endif
)

// int $div = spiGetClockDiv(int $spi)
IMPLEMENT_H(spiGetClockDiv, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiGetClockDiv(%d)\r\n",a0);
#else
  ph7_result_int64(pCtx, (sxi64)spiGetClockDiv(pHandle));
#endif
)

// int $mode = spiGetDataMode(int $spi)
IMPLEMENT_H(spiGetDataMode, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiGetDataMode(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, (int)spiGetDataMode(pHandle));
#endif
)

// int $order = spiGetBitOrder(int $spi)
IMPLEMENT_H(spiGetBitOrder, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiGetBitOrder(%d)\r\n",a0);
#else
  ph7_result_int(pCtx, (int)spiGetBitOrder(pHandle));
#endif
)

// spiSetClockDiv(int $spi, int $clock_div): void
IMPLEMENT_H_N(spiSetClockDiv, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSetClockDiv(%d, 0x%llx)\r\n",a0,a1);
#else
  spiSetClockDiv(pHandle, (uint32_t)a1);
#endif
)

// spiSetDataMode(int $spi, int $mode): void
IMPLEMENT_H_N(spiSetDataMode, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSetDataMode(%d, %d)\r\n",a0,a1);
#else
  spiSetDataMode(pHandle, (uint8_t)a1);
#endif
)

// spiSetBitOrder(int $spi, int $order): void
IMPLEMENT_H_N(spiSetBitOrder, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSetBitOrder(%d, %d)\r\n",a0,a1);
#else
  spiSetBitOrder(pHandle, (uint8_t)a1);
#endif
)

// int $div = spiFrequencyToClockDiv(int $spi, int $freq_hz)
IMPLEMENT_H_N(spiFrequencyToClockDiv, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiFrequencyToClockDiv(%d, %lld)\r\n",a0,a1);
#else
  ph7_result_int64(pCtx, (sxi64)spiFrequencyToClockDiv(pHandle, (uint32_t)a1));
#endif
)

// int $freq_hz = spiClockDivToFrequency(int $spi, int $clock_div)
IMPLEMENT_H_N(spiClockDivToFrequency, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiClockDivToFrequency(%d, 0x%llx)\r\n",a0,a1);
#else
  ph7_result_int64(pCtx, (sxi64)spiClockDivToFrequency(pHandle, (uint32_t)a1));
#endif
)

// spiWriteByte(int $spi, int $data): void
IMPLEMENT_H_N(spiWriteByte, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteByte(%d, 0x%x)\r\n",a0,a1);
#else
  spiWriteByte(pHandle, (uint8_t)a1);
#endif
)

// spiWriteWord(int $spi, int $data): void       16 bits
IMPLEMENT_H_N(spiWriteWord, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteWord(%d, 0x%x)\r\n",a0,a1);
#else
  spiWriteWord(pHandle, (uint16_t)a1);
#endif
)

// spiWriteLong(int $spi, int $data): void       32 bits
IMPLEMENT_H_N(spiWriteLong, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteLong(%d, 0x%llx)\r\n",a0,a1);
#else
  spiWriteLong(pHandle, (uint32_t)a1);
#endif
)

// int $in = spiTransferByte(int $spi, int $data)
IMPLEMENT_H_N(spiTransferByte, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferByte(%d, 0x%x)\r\n",a0,a1);
#else
  ph7_result_int(pCtx, (int)spiTransferByte(pHandle, (uint8_t)a1));
#endif
)

// int $in = spiTransferWord(int $spi, int $data)       16 bits
IMPLEMENT_H_N(spiTransferWord, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferWord(%d, 0x%x)\r\n",a0,a1);
#else
  ph7_result_int(pCtx, (int)spiTransferWord(pHandle, (uint16_t)a1));
#endif
)

// int $in = spiTransferLong(int $spi, int $data)       32 bits
IMPLEMENT_H_N(spiTransferLong, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferLong(%d, 0x%llx)\r\n",a0,a1);
#else
  ph7_result_int64(pCtx, (sxi64)spiTransferLong(pHandle, (uint32_t)a1));
#endif
)

// int $in = spiTransferBits(int $spi, int $data, int $bits)
// Original: spiTransferBits(spi, data, uint32_t *out, bits)
IMPLEMENT_H_N_N(spiTransferBits, spi_t, esp32_spi_get, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferBits(%d, 0x%llx, %d)\r\n",a0,a1,a2);
#else
  uint32_t nIn = 0;
  spiTransferBits(pHandle, (uint32_t)a1, &nIn, (uint8_t)a2);
  ph7_result_int64(pCtx, (sxi64)nIn);
#endif
)



#define ESP32_SPI_WORDS_MAX   16

// spiWrite(int $spi, string $data): void
// $data is 4 .. 64 bytes, a multiple of 4: that is 1 .. 16 words, sent in memory order.
//
IMPLEMENT_H_S(spiWrite, spi_t, esp32_spi_get,
  if (nLen < 4 || nLen > ESP32_SPI_WORDS_MAX * 4 || (nLen % 4) != 0) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiWrite: data must be 4..64 bytes, a multiple of 4");
    ph7_result_bool(pCtx, 0);
  } else {
#ifdef __CYGWIN__
    fprintf(stderr,"spiWrite(%d, <%d bytes>)\r\n",a0,nLen);
#else
    uint32_t aWords[ESP32_SPI_WORDS_MAX];
    memcpy(aWords, zData, (size_t)nLen);
    spiWrite(pHandle, aWords, (uint8_t)(nLen / 4));
#endif
  }
)

// string $in = spiTransfer(int $spi, string $data)
// Same rules as spiWrite(): $data is 4..64 bytes, a multiple of 4. Returns the same number of bytes.
// Original: spiTransfer(spi, uint32_t *out, len): in place
//
IMPLEMENT_H_S(spiTransfer, spi_t, esp32_spi_get,
  if (nLen < 4 || nLen > ESP32_SPI_WORDS_MAX * 4 || (nLen % 4) != 0) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiTransfer: data must be 4..64 bytes, a multiple of 4");
    ph7_result_bool(pCtx, 0);
  } else {
#ifdef __CYGWIN__
    fprintf(stderr,"spiTransfer(%d, <%d bytes>)\r\n",a0,nLen);
#else
    uint32_t aWords[ESP32_SPI_WORDS_MAX];
    memcpy(aWords, zData, (size_t)nLen);
    spiTransfer(pHandle, aWords, (uint8_t)(nLen / 4));
    ph7_result_string(pCtx, (const char *)aWords, nLen);
#endif
  }
)

/* spiTransferBytes() and spiTransferBytesNL(): string in, string of the same length out */
static int esp32_spi_xfer(ph7_context *pCtx, int nArg, ph7_value **apArg, int bNoLock, const char *zError) {

  spi_t *pSpi = NULL;

  if (nArg >= 2 && ph7_value_is_numeric(apArg[0]) && ph7_value_is_string(apArg[1]))
    pSpi = esp32_spi_get(ph7_value_to_int(apArg[0]));

  if (pSpi == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, zError);
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  int         nLen  = 0;
  const char *zData = ph7_value_to_string(apArg[1], &nLen);

  if (nLen < 1 || nLen > ESP32_IO_BUFFER_MAX) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "spiTransferBytes: data must be 1..65536 bytes");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  (void)bNoLock; (void)zData;

#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferBytes%s(%d, <%d bytes>)\r\n", bNoLock ? "NL" : "", ph7_value_to_int(apArg[0]), nLen);
#else
  uint8_t *pOut = (uint8_t *)malloc((size_t)nLen);
  if (pOut == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_ERR, "spiTransferBytes: out of memory");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  if (bNoLock)
    spiTransferBytesNL(pSpi, zData, pOut, (uint32_t)nLen);
  else
    spiTransferBytes(pSpi, (const uint8_t *)zData, pOut, (uint32_t)nLen);
  ph7_result_string(pCtx, (const char *)pOut, nLen);
  free(pOut);
#endif
  return PH7_OK;
}

// string $in = spiTransferBytes(int $spi, string $data)
// Original: spiTransferBytes(spi, data, uint8_t *out, size)
//
Impl(spiTransferBytes, pCtx, nArg, apArg) {
  return esp32_spi_xfer(pCtx, nArg, apArg, 0, "spiTransferBytes: expected (int $spi, string $data) and a valid handle");
}

/* ---- transactions and the "no lock" functions ---- */

// spiTransaction(int $spi, int $clock_div, int $data_mode, int $bit_order): void
IMPLEMENT_H_N_N_N(spiTransaction, spi_t, esp32_spi_get, long long, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransaction(%d, 0x%llx, %d, %d)\r\n",a0,a1,a2,a3);
#else
  spiTransaction(pHandle, (uint32_t)a1, (uint8_t)a2, (uint8_t)a3);
#endif
)

// spiSimpleTransaction(int $spi): void
IMPLEMENT_H(spiSimpleTransaction, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiSimpleTransaction(%d)\r\n",a0);
#else
  spiSimpleTransaction(pHandle);
#endif
)

// spiEndTransaction(int $spi): void
IMPLEMENT_H(spiEndTransaction, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiEndTransaction(%d)\r\n",a0);
#else
  spiEndTransaction(pHandle);
#endif
)

// spiWriteNL(int $spi, string $data): void
IMPLEMENT_H_S(spiWriteNL, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteNL(%d, <%d bytes>)\r\n",a0,nLen);
#else
  spiWriteNL(pHandle, zData, (uint32_t)nLen);
#endif
)

// spiWritePixelsNL(int $spi, string $data): void       16 bit pixels, byte swapped on the way out
IMPLEMENT_H_S(spiWritePixelsNL, spi_t, esp32_spi_get,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWritePixelsNL(%d, <%d bytes>)\r\n",a0,nLen);
#else
  spiWritePixelsNL(pHandle, zData, (uint32_t)nLen);
#endif
)

// spiWriteByteNL(int $spi, int $data): void
IMPLEMENT_H_N(spiWriteByteNL, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteByteNL(%d, 0x%x)\r\n",a0,a1);
#else
  spiWriteByteNL(pHandle, (uint8_t)a1);
#endif
)

// spiWriteShortNL(int $spi, int $data): void       16 bits
IMPLEMENT_H_N(spiWriteShortNL, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteShortNL(%d, 0x%x)\r\n",a0,a1);
#else
  spiWriteShortNL(pHandle, (uint16_t)a1);
#endif
)

// spiWriteLongNL(int $spi, int $data): void        32 bits
IMPLEMENT_H_N(spiWriteLongNL, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiWriteLongNL(%d, 0x%llx)\r\n",a0,a1);
#else
  spiWriteLongNL(pHandle, (uint32_t)a1);
#endif
)

// int $in = spiTransferByteNL(int $spi, int $data)
IMPLEMENT_H_N(spiTransferByteNL, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferByteNL(%d, 0x%x)\r\n",a0,a1);
#else
  ph7_result_int(pCtx, (int)spiTransferByteNL(pHandle, (uint8_t)a1));
#endif
)

// int $in = spiTransferShortNL(int $spi, int $data)       16 bits
IMPLEMENT_H_N(spiTransferShortNL, spi_t, esp32_spi_get, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferShortNL(%d, 0x%x)\r\n",a0,a1);
#else
  ph7_result_int(pCtx, (int)spiTransferShortNL(pHandle, (uint16_t)a1));
#endif
)

// int $in = spiTransferLongNL(int $spi, int $data)        32 bits
IMPLEMENT_H_N(spiTransferLongNL, spi_t, esp32_spi_get, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferLongNL(%d, 0x%llx)\r\n",a0,a1);
#else
  ph7_result_int64(pCtx, (sxi64)spiTransferLongNL(pHandle, (uint32_t)a1));
#endif
)

// string $in = spiTransferBytesNL(int $spi, string $data)
Impl(spiTransferBytesNL, pCtx, nArg, apArg) {
  return esp32_spi_xfer(pCtx, nArg, apArg, 1, "spiTransferBytesNL: expected (int $spi, string $data) and a valid handle");
}

// int $in = spiTransferBitsNL(int $spi, int $data, int $bits)
IMPLEMENT_H_N_N(spiTransferBitsNL, spi_t, esp32_spi_get, long long, int,
#ifdef __CYGWIN__
  fprintf(stderr,"spiTransferBitsNL(%d, 0x%llx, %d)\r\n",a0,a1,a2);
#else
  uint32_t nIn = 0;
  spiTransferBitsNL(pHandle, (uint32_t)a1, &nIn, (uint8_t)a2);
  ph7_result_int64(pCtx, (sxi64)nIn);
#endif
)


/*
 * RMT
 *
 * Only the functions without data buffers are here. rmtWrite*(), rmtRead*() are not
 * implemented yet: the RMT driver keeps using the caller symbol buffer AFTER the call
 * returns (timeout, async, looping), so a PHP string can not be passed as is.
 * TODO: brainstorm this
 */

// bool rmtInit(int $pin, int $direction, int $mem_blocks, int $freq_hz)       $direction: RMT_RX_MODE 0, RMT_TX_MODE 1
//
IMPLEMENT_N_N_N_N(rmtInit, int, int, int, long long,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtInit(%d, %d, %d, %lld)\r\n",a0,a1,a2,a3);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtInit(a0, a1, a2, (uint32_t)a3));
#else
  NOT_SUPPORTED(rmtInit);
#endif
#endif
)

// bool rmtDeinit(int $pin)
//
IMPLEMENT_N(rmtDeinit, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtDeinit(%d)\r\n",a0);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtDeinit(a0));
#else
  NOT_SUPPORTED(rmtDeinit);
#endif
#endif
)

// bool rmtSetEOT(int $pin, int $level)
//
IMPLEMENT_N_N(rmtSetEOT, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtSetEOT(%d, %d)\r\n",a0,a1);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtSetEOT(a0, (uint8_t)a1));
#else
  NOT_SUPPORTED(rmtSetEOT);
#endif
#endif
)

// bool rmtSetCarrier(int $pin, bool $carrier_en, bool $carrier_level, int $freq_hz, float $duty)
// $duty is 0.0 .. 1.0: 38.5 kHz 33%  =>  rmtSetCarrier($pin, true, true, 38500, 0.33)
//
IMPLEMENT_N_N_N_N_N(rmtSetCarrier, int, int, int, long long, float,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtSetCarrier(%d, %d, %d, %lld, %f)\r\n",a0,a1,a2,a3,(double)a4);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtSetCarrier(a0, a1 != 0, a2 != 0, (uint32_t)a3, a4));
#else
  NOT_SUPPORTED(rmtSetCarrier);
#endif
#endif
)

// bool rmtSetRxMaxThreshold(int $pin, int $idle_ticks)
//
IMPLEMENT_N_N(rmtSetRxMaxThreshold, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtSetRxMaxThreshold(%d, %d)\r\n",a0,a1);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtSetRxMaxThreshold(a0, (uint16_t)a1));
#else
  NOT_SUPPORTED(rmtSetRxMaxThreshold);
#endif
#endif
)

// bool rmtSetRxMinThreshold(int $pin, int $filter_ticks)
//
IMPLEMENT_N_N(rmtSetRxMinThreshold, int, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtSetRxMinThreshold(%d, %d)\r\n",a0,a1);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtSetRxMinThreshold(a0, (uint8_t)a1));
#else
  NOT_SUPPORTED(rmtSetRxMinThreshold);
#endif
#endif
)

// bool rmtTransmitCompleted(int $pin)
//
IMPLEMENT_N(rmtTransmitCompleted, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtTransmitCompleted(%d)\r\n",a0);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtTransmitCompleted(a0));
#else
  NOT_SUPPORTED(rmtTransmitCompleted);
#endif
#endif
)

// bool rmtReceiveCompleted(int $pin)
//
IMPLEMENT_N(rmtReceiveCompleted, int,
#ifdef __CYGWIN__
  fprintf(stderr,"rmtReceiveCompleted(%d)\r\n",a0);
#else
#if SOC_RMT_SUPPORTED
  ph7_result_bool(pCtx, rmtReceiveCompleted(a0));
#else
  NOT_SUPPORTED(rmtReceiveCompleted);
#endif
#endif
)


/* Interrupts: TODO:
 *
 * Handlers are PHP callables.
 */

// attachInterrupt(int $pin, callable $handler, int $mode): void
//
Impl(attachInterrupt, pCtx, nArg, apArg) {

  if (nArg < 3 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1]) ||
      !ph7_value_is_numeric(apArg[2])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "attachInterrupt: expected (int $pin, callable $handler, int $mode)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int  iPin     = ph7_value_to_int(apArg[0]);
  ph7_value *pHandler = apArg[1];
  const int  iMode    = ph7_value_to_int(apArg[2]);

  (void)iPin; (void)pHandler; (void)iMode;

#ifdef __CYGWIN__
  fprintf(stderr,"attachInterrupt(%d, <callable>, %d)\r\n", iPin, iMode);
#else
  /* TODO(ISR): attachInterrupt() */
#endif
  return PH7_OK;
}

// attachInterruptArg(int $pin, callable $handler, mixed $arg, int $mode): void
//
Impl(attachInterruptArg, pCtx, nArg, apArg) {

  if (nArg < 4 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1]) ||
      !ph7_value_is_numeric(apArg[3])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "attachInterruptArg: expected (int $pin, callable $handler, mixed $arg, int $mode)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int  iPin     = ph7_value_to_int(apArg[0]);
  ph7_value *pHandler = apArg[1];
  ph7_value *pUserArg = apArg[2];
  const int  iMode    = ph7_value_to_int(apArg[3]);

#ifdef __CYGWIN__
  fprintf(stderr,"attachInterruptArg(%d, <callable>, <arg>, %d)\r\n", iPin, iMode);
#else
  /* TODO(ISR): attachInterruptArg() */
#endif
  return PH7_OK;
}

// detachInterrupt(int $pin): void
//
IMPLEMENT_N(detachInterrupt, int,
#ifdef __CYGWIN__
  fprintf(stderr,"detachInterrupt(%d)\r\n",a0);
#else
  /* TODO(ISR): detachInterrupt() */
#endif
)

// enableInterrupt(int $pin): void
//
IMPLEMENT_N(enableInterrupt, int,
#ifdef __CYGWIN__
  fprintf(stderr,"enableInterrupt(%d)\r\n",a0);
#else
  /* TODO(ISR): enableInterrupt() */
#endif
)

// disableInterrupt(int $pin): void
//
IMPLEMENT_N(disableInterrupt, int,
#ifdef __CYGWIN__
  fprintf(stderr,"disableInterrupt(%d)\r\n",a0);
#else
  /* TODO(ISR): disableInterrupt() */
#endif
)

// touchAttachInterrupt(int $pin, callable $handler, int $threshold): void
//
Impl(touchAttachInterrupt, pCtx, nArg, apArg) {

  if (nArg < 3 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1]) ||
      !ph7_value_is_numeric(apArg[2])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "touchAttachInterrupt: expected (int $pin, callable $handler, int $threshold)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iPin       = ph7_value_to_int(apArg[0]);
  ph7_value  *pHandler   = apArg[1];
  const sxi64 nThreshold = ph7_value_to_int64(apArg[2]);

#ifdef __CYGWIN__
  fprintf(stderr,"touchAttachInterrupt(%d, <callable>, %lld)\r\n", iPin, (long long)nThreshold);
#else
  /* TODO(ISR): touchAttachInterrupt() */
#endif
  return PH7_OK;
}

// touchAttachInterruptArg(int $pin, callable $handler, mixed $arg, int $threshold): void
//
Impl(touchAttachInterruptArg, pCtx, nArg, apArg) {

  if (nArg < 4 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1]) ||
      !ph7_value_is_numeric(apArg[3])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "touchAttachInterruptArg: expected (int $pin, callable $handler, mixed $arg, int $threshold)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int   iPin       = ph7_value_to_int(apArg[0]);
  ph7_value  *pHandler   = apArg[1];
  ph7_value  *pUserArg   = apArg[2];
  const sxi64 nThreshold = ph7_value_to_int64(apArg[3]);

#ifdef __CYGWIN__
  fprintf(stderr,"touchAttachInterruptArg(%d, <callable>, <arg>, %lld)\r\n", iPin, (long long)nThreshold);
#else
  /* TODO(ISR): touchAttachInterruptArg() */
#endif
  return PH7_OK;
}

// touchDetachInterrupt(int $pin): void
//
IMPLEMENT_N(touchDetachInterrupt, int,
#ifdef __CYGWIN__
  fprintf(stderr,"touchDetachInterrupt(%d)\r\n",a0);
#else
  /* TODO(ISR): touchDetachInterrupt() */
#endif
)

// timerAttachInterrupt(int $timer, callable $handler): void
// $timer is the handle returned by timerBegin(): resolve it with esp32_timer_get()
//
Impl(timerAttachInterrupt, pCtx, nArg, apArg) {

  if (nArg < 2 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerAttachInterrupt: expected (int $timer, callable $handler)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int  iTimer   = ph7_value_to_int(apArg[0]);
  ph7_value *pHandler = apArg[1];

#ifdef __CYGWIN__
  fprintf(stderr,"timerAttachInterrupt(%d, <callable>)\r\n", iTimer);
#else
  /* TODO(ISR): timerAttachInterrupt() */
#endif
  return PH7_OK;
}

// timerAttachInterruptArg(int $timer, callable $handler, mixed $arg): void
//
Impl(timerAttachInterruptArg, pCtx, nArg, apArg) {

  if (nArg < 3 ||
      !ph7_value_is_numeric(apArg[0]) ||
      !ph7_value_is_callable(apArg[1])) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "timerAttachInterruptArg: expected (int $timer, callable $handler, mixed $arg)");
    ph7_result_null(pCtx);
    return PH7_OK;
  }

  const int  iTimer   = ph7_value_to_int(apArg[0]);
  ph7_value *pHandler = apArg[1];
  ph7_value *pUserArg = apArg[2];

  (void)iTimer; (void)pHandler; (void)pUserArg;

#ifdef __CYGWIN__
  fprintf(stderr,"timerAttachInterruptArg(%d, <callable>, <arg>)\r\n", iTimer);
#else
  /* TODO(ISR): timerAttachInterruptArg() */
#endif
  return PH7_OK;
}

// timerDetachInterrupt(int $timer): void
//
IMPLEMENT_N(timerDetachInterrupt, int,
#ifdef __CYGWIN__
  fprintf(stderr,"timerDetachInterrupt(%d)\r\n",a0);
#else
  /* TODO(ISR): timerDetachInterrupt() */
#endif
)

#endif //#ifndef PH7_DISABLE_ESP32_ARDUINO_FUNC

#ifndef PH7_DISABLE_ESP32_ESPIDF_FUNC
#if 0

$config = [
  'fun_sel' => 0,                   /*!< Value of IOMUX function selection */
  'sig_out' => 0,                   /*!< Index of the outputting peripheral signal */ 
  'drv' => 0,                       /*!< Value of drive strength */
  'pu' => 0,                        /*!< Status of pull-up enabled or not */
  'pd' => 0,                        /*!< Status of pull-down enabled or not */
  'ie' => 0,                        /*!< Status of input enabled or not */
  'oe' => 0,                        /*!< Status of output enabled or not */
  'oe_ctrl_by_periph' => 0,          /*!< True if use output enable signal from peripheral, otherwise False */
  'oe_inv' => 0,                     /*!< Whether the output enable signal is inversed or not */
  'od' => 0,                         /*!< Status of open-drain enabled or not */
  'slp_sel' => 0                     /*!< Status of pin sleep mode enabled or not */
];

function gpio_config($pin, $cfg);

int Impl(gpio_config(ph7_context *ctx, int argc, ph7_value **argv) {  return PH7_OK; }

int Impl(gpio_reset_pin(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_get_io_config(ph7_context *ctx, int argc, ph7_value **argv)  { return PH7_OK; }

int Impl(gpio_get_drive_capability(ph7_context *ctx, int argc, ph7_value **argv) { return PH7_OK; }
int Impl(gpio_set_drive_capability(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_get_level(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_set_level(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_set_direction(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_iomux_input(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_iomux_output(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_func_sel(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_pulldown_dis(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_pulldown_en(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_pullup_dis(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_pullup_en(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_set_pull_mode(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_od_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_od_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_input_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_input_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_output_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_output_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_sleep_sel_dis(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_sleep_sel_en(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_sleep_set_direction(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_sleep_set_pull_mode(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_hold_dis(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_hold_en(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_deep_sleep_hold_dis(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_deep_sleep_hold_en(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_deep_sleep_wakeup_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_deep_sleep_wakeup_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_force_hold_all(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_force_unhold_all(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_wakeup_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_wakeup_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

int Impl(gpio_install_isr_service(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_uninstall_isr_service(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_intr_disable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_intr_enable(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_isr_handler_add(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_isr_handler_remove(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_isr_register(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }
int Impl(gpio_set_intr_type(ph7_context *ctx, int argc, ph7_value **argv) {   return PH7_OK; }

#endif // 0
#endif // #ifndef PH7_DISABLE_ESP32_ESPIDF_FUNC


#ifndef PH7_DISABLE_ESP32_FREERTOS_FUNC
#if 0
// void vTaskDelay(const TickType_t xTicksToDelay);
//
int Impl(vTaskDelay(ph7_context *ctx, int argc, ph7_value **argv) {
  return PH7_OK;
}

// Queues are implemented as PHP-only queues (you can not pass PHP's Queue_t handle to a foreign C function)
// see FreeRTOS.php

//BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify,
//                       uint32_t ulValue,
//                       eNotifyAction eAction);
//
int Impl( xTaskNotify )(ph7_context *ctx, int argc, ph7_value **argv) {
  return PH7_OK;
}


//BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry,
//                           uint32_t ulBitsToClearOnExit,
//                           uint32_t *pulNotificationValue,
//                           TickType_t xTicksToWait);
//
int Impl( xTaskNotifyWait )(ph7_context *ctx, int argc, ph7_value **argv) {
  return PH7_OK;
}


//BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);
//
int Impl( xTaskNotifyGive )(ph7_context *ctx, int argc, ph7_value **argv) {
  return PH7_OK;
}

//uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit,
//                          TickType_t xTicksToWait);
//
int Impl( ulTaskNotifyTake )(ph7_context *ctx, int argc, ph7_value **argv) {
  return PH7_OK;
}
#endif //#if 0
#endif //#ifndef PH7_DISABLE_ESP32_FREERTOS_FUNC


#undef Impl






