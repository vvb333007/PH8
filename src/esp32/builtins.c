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
 * 
 *
 */
#include "ph7int.h"
#include "builtins.h"


/* Templates and boilerplates.
 * IMPLEMENT() macros are function templates: 0, 1 or two args.
 *
 *  C11 Dispatcher, choose converter function by the type T 
 */
#define ph7_value_to(T, v) _Generic((T)0,          \
    int:              ph7_value_to_int,            \
    long:             ph7_value_to_int64,          \
    long long:        ph7_value_to_int64,          \
    float:            ph7_value_to_double,         \
    double:           ph7_value_to_double          \
)(v)

/* Low level template for foreign functions which can not be generalized into IMPLEMENT_N_N() 
 * E.g. function random() below can work with 1 or 2 arguments - means no suitable IMPLEMENT macro
 */
#define Impl( Name_ , pCtx_ , nArg_, apArg_) \
  int esp32_builtin_ ## Name_ (ph7_context *pCtx_, int nArg_, ph7_value **apArg_)

/* Two numeric arguments like digitalWrite(pin, value).
 * CODE must call ph7_result_* to set the return value.
 * if it doesnt the return value is whatever was in the accumulator. 
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

/* No-arg functions like millis(), micros(), esp_timer_get_time() 
*/
#define IMPLEMENT(name, ...)                                \
  Impl(name, pCtx, nArg, apArg) {                           \
    pCtx = pCtx; \
    (void)nArg; (void)apArg;                                \
    __VA_ARGS__;                                            \
    return PH7_OK;                                          \
  }

#ifndef PH7_DISABLE_ESP32_ARDUINO_FUNC
# ifdef ESP32
#  include <Arduino.h>
# endif

/* Actual implemetation.
 * IMPLEMENT_N_N(functionName, arg1 type, arg2 type, CODE...) will create a function
 * int ep32_builtin_functionName(...) which will be called each time PHP codes calls functionName($arg1, $arg2)
 *
 * The CODE.. part is the actual code to be executed; Arguments are available as a0, a1, a2, and so on
 */
IMPLEMENT_N_N(pinMode, int, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  pinMode(a0, a1);
#endif
)

IMPLEMENT_N_N(digitalWrite, int, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  digitalWrite(a0, a1 ? 1 : 0);
#endif
)

IMPLEMENT_N(delay, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  delay(a0);
#endif
)

IMPLEMENT_N(delayMicroseconds, long long,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  delayMicroseconds(a0);
#endif
)

IMPLEMENT_N(digitalRead, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int(pCtx, digitalRead(a0));
#endif
)

IMPLEMENT(millis,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int64(pCtx, (sxi64)millis());
#endif
)

IMPLEMENT(micros,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int64(pCtx, (sxi64)micros());
#endif
)

// bool $ret = digitalPinIsValid($pin)
//
IMPLEMENT_N(digitalPinIsValid, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int(pCtx, digitalPinIsValid(a0));
#endif
)

// bool digitalPinCanOutput($pin)
//
IMPLEMENT_N(digitalPinCanOutput, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int(pCtx, digitalPinCanOutput(a0));
#endif
)


// $rtc_pin = digitalPinToRtcPin($pin)
//
IMPLEMENT_N(digitalPinToRtcPin, int,
#ifdef __CYGWIN__
  puts("pinMode()");
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
  puts("pinMode()");
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
  puts("pinMode()");
#else
  ph7_result_int(pCtx, digitalPinToTouchChannel(a0));
#endif
)

// $chan = digitalPinToAnalogChannel($pin)
//
IMPLEMENT_N(digitalPinToAnalogChannel, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int(pCtx, digitalPinToAnalogChannel(a0));
#endif
)

// $digi_pin = analogChannelToDigitalPin($ana_pin);
//
IMPLEMENT_N(analogChannelToDigitalPin, int,
#ifdef __CYGWIN__
  puts("pinMode()");
#else
  ph7_result_int(pCtx, analogChannelToDigitalPin(a0));
#endif
)

// shiftIn(dataPin, clockPin, bitOrder)
// TODO: implement
//
IMPLEMENT(shiftIn,
  /* not implemented yet */
)

// shiftOut(dataPin, clockPin, bitOrder, value)
// TODO: implement
//
IMPLEMENT(shiftOut,
  /* not implemented yet */
)

// pulseIn()
// TODO: implement
//
IMPLEMENT(pulseIn,
  /* not implemented yet */
)

// pulseInLong
// TODO: implement
//
IMPLEMENT(pulseInLong,
  /* not implemented yet */
)

// $res = makeWord($a)
// $res = makeWord($a, $b)
// TODO: implement
//
IMPLEMENT(makeWord,
  /* not implemented yet */
)


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

// Too many args to create a dedicated IMPLEMENT_N_N_N_N...
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
    puts("random() called");
#else
    ph7_result_int(pCtx, min + (long)(esp_random() % (unsigned long)(max - min)));
#endif
  }

  return PH7_OK;
}

#endif //#ifndef PH7_DISABLE_ESP32_ARDUINO_FUNC

#ifndef PH7_DISABLE_ESP32_ESPIDF_FUNC
#if 0
/* TODO: should it be a class instance to make the syntax closer to C++ or leave it as a hasmap aka array?
 *
 */
$config = new gpio_config_t;

$config->fun_sel;                   /*!< Value of IOMUX function selection */
$config->sig_out;                   /*!< Index of the outputting peripheral signal */
$config->drv;                       /*!< Value of drive strength */
$config->pu;                        /*!< Status of pull-up enabled or not */
$config->pd;                        /*!< Status of pull-down enabled or not */
$config->ie;                        /*!< Status of input enabled or not */
$config->oe;                        /*!< Status of output enabled or not */
$config->oe_ctrl_by_periph;          /*!< True if use output enable signal from peripheral, otherwise False */
$config->oe_inv;                     /*!< Whether the output enable signal is inversed or not */
$config->od;                         /*!< Status of open-drain enabled or not */
$config->slp_sel;                     /*!< Status of pin sleep mode enabled or not */



// Function that require a structure as an argument are implemented partially in PHP.
// The reason for this is to simplify binding function code: we defenitely do not want to
// parse arrays or classes in .c code. Instead we "vectorize" calls where needed
//
// function gpio_config($pin, $cfg) {
//   /* .c foreign function which assembles a structure from arguments and calls ESP-IDF's gpio_config */
//   gpio_config_edf( $pin,
//                    $c->fun_sel,
//                    $c->sig_out,
//                    $c->drv,
//                    $c->pu,
//                    $c->pd,
//                    $c->ie,
//                    $c->oe,
//                    $c->oe_ctrl_by_periph,
//                    $c->oe_inv,
//                    $c->od,
//                    $c->slp_sel);
//
// }
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






