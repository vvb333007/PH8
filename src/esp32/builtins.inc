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

/* Arduino Core / ESP-IDF native bindigs registration is done here
 * This file is then included by PH7 builtin.c to register Arduino functions as built-ins
 *
 * To add a new function (or to delete the existing one) all 3 files (builtins.c, .h and .inc)
 * must be updated and kept in sync
 */

#define Bind(Name_) \
  { #Name_,  esp32_builtin_ ## Name_ }, 

#ifndef PH7_DISABLE_ESP32_ARDUINO_FUNC

// References: https://docs.arduino.cc/language-reference/
//             Arduino Core 3.x source code
//             


// Advanced I/O
//
Bind( shiftIn )
Bind( shiftOut )
Bind( pulseIn )
Bind( pulseInLong )

//Bind( tone )
//Bind( noTone )

// Characters
//
Bind( isAlpha )
Bind( isAlphaNumeric )
Bind( isAscii )
Bind( isControl )
Bind( isDigit )
Bind( isGraph )
Bind( isHexadecimalDigit )
Bind( isLowerCase )
Bind( isPrintable )
Bind( isPunct )
Bind( isSpace )
Bind( isUpperCase )
Bind( isWhitespace )
Bind( toAscii )
Bind( toLowerCase )
Bind( toUpperCase )

// Interrupts / External interrupts
//
//Bind( interrupts )
//Bind( noInterrupts )
Bind( attachInterrupt )
Bind( attachInterruptArg )
Bind( detachInterrupt )
Bind( enableInterrupt )
Bind( disableInterrupt )


// Analog IO
//
Bind( analogChannelToDigitalPin )
Bind( analogRead )
Bind( analogReadResolution )
Bind( analogWrite )
Bind( analogWriteResolution )
Bind( analogReadMilliVolts )
Bind( analogWriteFrequency )
Bind( analogSetAttenuation )
Bind( analogSetPinAttenuation )
{ "analogSetWidth",  esp32_builtin_analogReadResolution },  /* A synonym */
//Bind( analogContinuous )
//Bind( analogContinuousDeinit )
//Bind( analogContinuousRead )
//Bind( analogContinuousSetAtten )
//Bind( analogContinuousSetWidth )
//Bind( analogContinuousStart )
//Bind( analogContinuousStop )



// Random Numbers
Bind( random )
Bind( randomSeed )
//Bind( randomVector )

// Misc
Bind( map )
Bind( makeWord )
//Bind( constrain )

// Time
Bind( micros )
Bind( millis )
Bind( delay )
Bind( delayMicroseconds )


// Digital IO
Bind( pinMode )
Bind( digitalWrite )
Bind( digitalRead )
Bind( digitalPinIsValid )
Bind( digitalPinCanOutput )
Bind( digitalPinToRtcPin )
Bind( digitalPinToDacChannel )
Bind( digitalPinToTouchChannel )
Bind( digitalPinToAnalogChannel )

// PWM controller (LEDC)
//
Bind( ledcAttach )
Bind( ledcAttachChannel )
Bind( ledcChangeFrequency )
Bind( ledcDetach )
Bind( ledcFade )
//Bind( ledcGetClockSource )
Bind( ledcOutputInvert )
Bind( ledcRead )
Bind( ledcReadFreq )
//Bind( ledcSetClockSource )
Bind( ledcWrite )
Bind( ledcWriteChannel )
Bind( ledcWriteNote )
Bind( ledcWriteTone )

// I2C
//
Bind( i2cDeinit )
Bind( i2cGetClock )
Bind( i2cInit )
Bind( i2cIsInit )
Bind( i2cRead )
Bind( i2cSetClock )
Bind( i2cWrite )
Bind( i2cWriteReadNonStop )

// CPU
//
Bind( getApbFrequency )
Bind( getCpuFrequencyMhz )
Bind( getXtalFrequencyMhz )
Bind( setCpuFrequencyMhz )
Bind( temperatureRead )


// Remote Control Peripheral (RMT)
//
Bind( rmtDeinit )
Bind( rmtInit )
//Bind( rmtRead )
//Bind( rmtReadAsync )
Bind( rmtReceiveCompleted )
Bind( rmtSetCarrier )
Bind( rmtSetEOT )
Bind( rmtSetRxMaxThreshold )
Bind( rmtSetRxMinThreshold )
Bind( rmtTransmitCompleted )
//Bind( rmtWrite )
//Bind( rmtWriteAsync )
//Bind( rmtWriteLooping )
//Bind( rmtWriteRepeated )


// Sigma-Delta
//
Bind( sigmaDeltaAttach )
Bind( sigmaDeltaDetach )
Bind( sigmaDeltaWrite )


// Touch
//
Bind( touchAttachInterrupt )
Bind( touchAttachInterruptArg )
Bind( touchDetachInterrupt )
//Bind( touchInterruptGetLastStatus )
//Bind( touchInterruptSetThresholdDirection )
Bind( touchRead )
//Bind( touchSetConfig )
//Bind( touchSetDefaultThreshold )
//Bind( touchSetTiming )
//Bind( touchSleepWakeUpEnable )


Bind( timerBegin )
Bind( timerEnd )
Bind( timerStart )
Bind( timerStop )
Bind( timerRestart )
Bind( timerWrite )
Bind( timerRead )
Bind( timerReadMicros )
Bind( timerReadMillis )
Bind( timerReadSeconds )
Bind( timerGetFrequency )
Bind( timerAttachInterrupt )
Bind( timerAttachInterruptArg )
Bind( timerDetachInterrupt )
Bind( timerAlarm )

Bind( uartAvailable )
Bind( uartAvailableForWrite )
Bind( uartBegin )
Bind( uartDetectBaudrate )
Bind( uartEnableRxInternalPull )
Bind( uartEnd )
Bind( uartFlush )
Bind( uartFlushTxOnly )
Bind( uartGetBaudRate )
Bind( uartGetDebug )
Bind( uartIsDriverInstalled )
Bind( uartPeek )
Bind( uartPinSignalInversion )
Bind( uartRead )
Bind( uartReadBytes )
Bind( uartSetBaudRate )
Bind( uartSetClockSource )
Bind( uartSetCtsInvert )
Bind( uartSetDebug )
Bind( uartSetFastReading )
Bind( uartSetHwFlowCtrlMode )
Bind( uartSetIrdaDirection )
Bind( uartSetMode )
Bind( uartSetPins )
Bind( uartSetRtsInvert )
Bind( uartSetRxFIFOFull )
Bind( uartSetRxInvert )
Bind( uartSetRxTimeout )
Bind( uartSetTxInvert )
Bind( uartStartDetectBaudrate )
Bind( uartWrite )
Bind( uartWriteBuf )

Bind( spiStartBus )
Bind( spiStopBus )
Bind( spiAttachSCK )
Bind( spiAttachMISO )
Bind( spiAttachMOSI )
Bind( spiAttachSS )
Bind( spiDetachSCK )
Bind( spiDetachMISO )
Bind( spiDetachMOSI )
Bind( spiDetachSS )
Bind( spiEnableSSPins )
Bind( spiDisableSSPins )
Bind( spiSSEnable )
Bind( spiSSDisable )
Bind( spiSSSet )
Bind( spiSSClear )
Bind( spiWaitReady )
Bind( spiSSInvert )
Bind( spiGetClockDiv )
Bind( spiGetDataMode )
Bind( spiGetBitOrder )
Bind( spiSetClockDiv )
Bind( spiSetDataMode )
Bind( spiSetBitOrder )
Bind( spiWrite )
Bind( spiWriteByte )
Bind( spiWriteWord )
Bind( spiWriteLong )
Bind( spiTransfer )
Bind( spiTransferByte )
Bind( spiTransferWord )
Bind( spiTransferLong )
Bind( spiTransferBytes )
Bind( spiTransferBits )
Bind( spiTransaction )
Bind( spiSimpleTransaction )
Bind( spiEndTransaction )
Bind( spiWriteNL )
Bind( spiWriteByteNL )
Bind( spiWriteShortNL )
Bind( spiWriteLongNL )
Bind( spiWritePixelsNL )
Bind( spiTransferByteNL )
Bind( spiTransferShortNL )
Bind( spiTransferLongNL )
Bind( spiTransferBytesNL )
Bind( spiTransferBitsNL )
Bind( spiFrequencyToClockDiv )
Bind( spiClockDivToFrequency )


#endif // Arduino Core bindings


/* FreeRTOS native bindigs, a subset
 */
#ifndef PH7_DISABLE_ESP32_FREERTOS_FUNC

//Bind( xQueueCreate )
//Bind( vQueueDelete )
//Bind( xQueueSend )
//Bind( xQueueReceive )

//Bind( xTaskNotify )
//Bind( xTaskNotifyWait )
//Bind( xTaskNotifyGive )
//Bind( ulTaskNotifyTake )

//Bind( vTaskDelay )


#endif // FreeRTOS



/* ESP-IDF native bindigs, a subset
 *
 */
#ifndef PH7_DISABLE_ESP32_ESPIDF_FUNC
//Bind( gpio_config )
//Bind( gpio_get_drive_capability )
//Bind( gpio_get_io_config )
//Bind( gpio_get_level )
//Bind( gpio_hold_dis )
//Bind( gpio_hold_en )
//Bind( gpio_install_isr_service )
//Bind( gpio_uninstall_isr_service )
//Bind( gpio_intr_disable )
//Bind( gpio_intr_enable )
//Bind( gpio_isr_handler_add )
//Bind( gpio_isr_handler_remove )
//Bind( gpio_isr_register )
//Bind( gpio_iomux_input )
//Bind( gpio_iomux_output )
//Bind( gpio_func_sel )
//Bind( gpio_pulldown_dis )
//Bind( gpio_pulldown_en )
//Bind( gpio_pullup_dis )
//Bind( gpio_pullup_en )
//Bind( gpio_od_enable )
//Bind( gpio_od_disable )
//Bind( gpio_input_enable )
//Bind( gpio_input_disable )
//Bind( gpio_output_enable )
//Bind( gpio_output_disable )
//Bind( gpio_reset_pin )
//Bind( gpio_set_direction )
//Bind( gpio_set_drive_capability )
//Bind( gpio_set_intr_type )
//Bind( gpio_set_level )
//Bind( gpio_set_pull_mode )
//Bind( gpio_sleep_sel_dis )
//Bind( gpio_sleep_sel_en )
//Bind( gpio_sleep_set_direction )
//Bind( gpio_sleep_set_pull_mode )
//Bind( gpio_deep_sleep_hold_dis )
//Bind( gpio_deep_sleep_hold_en )
//Bind( gpio_deep_sleep_wakeup_disable )
//Bind( gpio_deep_sleep_wakeup_enable )
//Bind( gpio_force_hold_all )
//Bind( gpio_force_unhold_all )
//Bind( gpio_wakeup_disable )
//Bind( gpio_wakeup_enable )
#endif // ESP-IDF native bindings

#undef Bind

