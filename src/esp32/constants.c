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

#include "ph7int.h"
/*
 * Implementation of xExpand callback for every constant >= 0; For constants which are < 0 use MinusConst
 *
 * Const(0) -- creates a function which returns 0
 * Const(0xff) -- returns 255
 * MinusConst(27) -- returns -27
 * MinusConst(0xff) -- returns -255
 *
 *
 * WARNING!!!: a Value_ argument MUST BE a simple number: 1,5, 0xffff, 0777; 
 * Not 89.3, -78, -0xff
 * 
 */

/* Templates. */
#define Const( Value_ ) \
void esp32_const_int ## Value_ ( ph7_value *pVal, void *pUserData ) { \
\
  SXUNUSED( pUserData ); \
  ph7_value_int( pVal, Value_ ); \
}

#define MinusConst( Value_ ) \
void esp32_const_mint ## Value_ ( ph7_value *pVal, void *pUserData ) { \
\
  SXUNUSED( pUserData ); \
  ph7_value_int( pVal, -Value_ ); \
}


/* Implementation */
Const(0)
Const(1)
Const(2)
Const(3)
Const(4)
Const(5)
Const(6)
Const(7)
Const(8)
Const(9)
Const(12)
Const(13)
Const(16)
Const(19)
Const(32)
Const(64)
Const(128)
Const(192)

MinusConst(1)



#undef Const
#undef MinusConst

void esp32_const_8n1( ph7_value *pVal, void *pUserData ) {

  SXUNUSED( pUserData ); 
  ph7_value_int( pVal, 0x800001c ); 
}


#if 0
#ifndef PH7_DISABLE_ESP32_ARDUINO_CONSTS

static void esp32_const_8n1( ph7_value *pVal, void *pUserData ) {

  SXUNUSED( pUserData ); 
  ph7_value_int( pVal, 0x800001c ); 
}


  /* Instead of using Arduino's constants we use raw numbers. This is required to be able to use shared handlers
   * If these constants are changed in ESP-IDF or Arduino Core then they must be changed here as well
   *
   */
static const ph7_builtin_constant aArduConsts[] = {
  /* Arduino ADC native constants 
  */
  { "ADC_0db", esp32_const_int0 },
  { "ADC_2_5db", esp32_const_int1 },
  { "ADC_6db", esp32_const_int2 },
  { "ADC_11db", esp32_const_int3 },
  { "ADC_ATTENDB_MAX", esp32_const_int4 }, //

  /*
  * Arduino GPIO native constants
  */
  { "LOW", esp32_const_int0 },               //  0x0
  { "HIGH", esp32_const_int1 },               // 0x1
  { "INPUT", esp32_const_int1 },             // 0x01
  { "OUTPUT", esp32_const_int3 }, //            0x03
  { "PULLUP", esp32_const_int4 }, //            0x04
  { "INPUT_PULLUP", esp32_const_int5 }, //      0x05
  { "PULLDOWN", esp32_const_int8 }, //          0x08
  { "INPUT_PULLDOWN", esp32_const_int9 }, //    0x09
  { "OPEN_DRAIN", esp32_const_int16 }, //        0x10
  { "OUTPUT_OPEN_DRAIN", esp32_const_int19 }, // 0x13
  { "ANALOG", esp32_const_int192 }, //            0xC0
  { "DISABLED", esp32_const_int0 },         //  0x00
  { "RISING", esp32_const_int1 },         //    0x01
  { "FALLING", esp32_const_int2 },         //   0x02
  { "CHANGE", esp32_const_int3 },         //    0x03
  { "ONLOW", esp32_const_int4 },         //     0x04
  { "ONHIGH", esp32_const_int5 },         //    0x05
  { "ONLOW_WE", esp32_const_int12 },         //  0x0C
  { "ONHIGH_WE", esp32_const_int13 },         // 0x0D

  /* Arduino SPI
   */
# ifdef CONFIG_IDF_TARGET_ESP32
  { "FSPI", esp32_const_int1 },
  { "HSPI", esp32_const_int2 },
  { "VSPI", esp32_const_int3 },
# else
  { "FSPI", esp32_const_int0 },
  { "HSPI", esp32_const_int1 },
# endif // Original ESP32

  { "SPI_CLOCK_DIV2",   esp32_const_int2 },   // 8 MHz
  { "SPI_CLOCK_DIV4",   esp32_const_int4 },   // 4 MHz
  { "SPI_CLOCK_DIV8",   esp32_const_int8 },   // 2 MHz
  { "SPI_CLOCK_DIV16",  esp32_const_int16 },  // 1 MHz
  { "SPI_CLOCK_DIV32",  esp32_const_int32 },  // 500 KHz
  { "SPI_CLOCK_DIV64",  esp32_const_int64 },  // 250 KHz
  { "SPI_CLOCK_DIV128", esp32_const_int128 }, // 125 KHz

  { "SPI_MODE0", esp32_const_int0 },
  { "SPI_MODE1", esp32_const_int1 },
  { "SPI_MODE2", esp32_const_int2 },
  { "SPI_MODE3", esp32_const_int3 },

  { "SPI_SS0",  esp32_const_int0 },
  { "SPI_SS1",  esp32_const_int1 },
  { "SPI_SS2",  esp32_const_int2 },
  { "SPI_SS_MASK_ALL", esp32_const_int7 },

  { "SPI_LSBFIRST", esp32_const_int0 },
  { "SPI_MSBFIRST", esp32_const_int1 },

  /* Arduino CPU
   *
   */
  { "APB_BEFORE_CHANGE", esp32_const_int0 },
  { "APB_AFTER_CHANGE", esp32_const_int1 },

  { "RGB_BRIGHTNESS", esp32_const_int64 },
  { "RGB_BUILTIN_LED_COLOR_ORDER", esp32_const_int5 },  // default WS2812B color order, GRB
  { "LED_COLOR_ORDER_RGB", esp32_const_int0 },
  { "LED_COLOR_ORDER_BGR", esp32_const_int1 },
  { "LED_COLOR_ORDER_BRG", esp32_const_int2 },
  { "LED_COLOR_ORDER_RBG", esp32_const_int3 },
  { "LED_COLOR_ORDER_GBR", esp32_const_int4 },
  { "LED_COLOR_ORDER_GRB", esp32_const_int5 },

  { "SERIAL_8N1", esp32_const_8n1 },

  { "UART_SIGNAL_INV_DISABLE", esp32_const_int0 },
  { "UART_SIGNAL_IRDA_TX_INV", esp32_const_int1 },
  { "UART_SIGNAL_IRDA_RX_INV", esp32_const_int2 },
  { "UART_SIGNAL_RXD_INV", esp32_const_int4 },
  { "UART_SIGNAL_CTS_INV", esp32_const_int8 },
  { "UART_SIGNAL_DSR_INV", esp32_const_int16 },
  { "UART_SIGNAL_TXD_INV", esp32_const_int32 },
  { "UART_SIGNAL_RTS_INV", esp32_const_int64 },
  { "UART_SIGNAL_DTR_INV", esp32_const_int128 },

  { "ESP32_UART_IRDA_RX", esp32_const_int0 },
  { "ESP32_UART_IRDA_TX", esp32_const_int1 },

  { "RMT_RX_MODE", esp32_const_int0 },
  { "RMT_TX_MODE", esp32_const_int1 },

  { "UART_MODE_UART", esp32_const_int0 },
  { "UART_MODE_RS485_HALF_DUPLEX", esp32_const_int1 },
  { "UART_MODE_IRDA", esp32_const_int2 },
  { "UART_MODE_RS485_COLLISION_DETECT", esp32_const_int3 },
  { "UART_MODE_RS485_APP_CTRL", esp32_const_int4 },

#endif // Disable Arduino constants

#ifndef PH7_DISABLE_ESP32_FREERTOS_CONSTS
  /*
  * FreeRTOS native constants
  */
  { "pdPASS",    esp32_const_int1 },
  { "pdFAIL",    esp32_const_int0 },
  { "portMAX_DELAY",    esp32_const_mint1 }, // Ugly hack. -1 is 0xffffffff

  { "eNoAction",    esp32_const_int0 },
  { "eSetBits",    esp32_const_int1 },
  { "eIncrement",    esp32_const_int2 },
  { "eSetValueWithOverwrite",    esp32_const_int3 },
  { "eSetValueWithoutOverwrite",    esp32_const_int4 },

#endif //#ifndef PH7_DISABLE_ESP32_FREERTOS_CONSTS

 /*
  * ESP-IDF native constants (GPIO ).
  */

#ifndef PH7_DISABLE_ESP32_ESPIDF_CONSTS
  { "GPIO_PORT_0",    esp32_const_int0 },
  { "GPIO_PORT_MAX",  esp32_const_int1 },

  { "GPIO_INTR_DISABLE",    esp32_const_int0 },
  { "GPIO_INTR_POSEDGE",    esp32_const_int1 },
  { "GPIO_INTR_NEGEDGE",    esp32_const_int2 },
  { "GPIO_INTR_ANYEDGE",    esp32_const_int3 },
  { "GPIO_INTR_LOW_LEVEL",  esp32_const_int4 },
  { "GPIO_INTR_HIGH_LEVEL", esp32_const_int5 },
  { "GPIO_INTR_MAX",        esp32_const_int6 },

  { "GPIO_MODE_DEF_DISABLE",esp32_const_int0 },
  { "GPIO_MODE_DEF_INPUT",  esp32_const_int1 },
  { "GPIO_MODE_DEF_OUTPUT", esp32_const_int2 },
  { "GPIO_MODE_DEF_OD",     esp32_const_int4 },


  { "GPIO_MODE_DISABLE",          esp32_const_int0 },
  { "GPIO_MODE_INPUT",            esp32_const_int1 },
  { "GPIO_MODE_OUTPUT",           esp32_const_int2 },
  { "GPIO_MODE_OUTPUT_OD",        esp32_const_int6 },
  { "GPIO_MODE_INPUT_OUTPUT_OD",  esp32_const_int7 },
  { "GPIO_MODE_INPUT_OUTPUT",     esp32_const_int3 },


  { "GPIO_PULLUP_DISABLE",  esp32_const_int0 },
  { "GPIO_PULLUP_ENABLE",   esp32_const_int1 },

  { "GPIO_PULLDOWN_DISABLE",esp32_const_int0 },
  { "GPIO_PULLDOWN_ENABLE", esp32_const_int1 },

  { "GPIO_PULLUP_ONLY",     esp32_const_int0 },
  { "GPIO_PULLDOWN_ONLY",   esp32_const_int1 },
  { "GPIO_PULLUP_PULLDOWN", esp32_const_int2 },
  { "GPIO_FLOATING",        esp32_const_int3 },

  { "GPIO_DRIVE_CAP_0",       esp32_const_int0 },
  { "GPIO_DRIVE_CAP_1",       esp32_const_int1 },
  { "GPIO_DRIVE_CAP_2",       esp32_const_int2 },
  { "GPIO_DRIVE_CAP_DEFAULT", esp32_const_int2 },
  { "GPIO_DRIVE_CAP_3",       esp32_const_int3 },
  { "GPIO_DRIVE_CAP_MAX",     esp32_const_int4 },
#endif // #ifndef PH7_DISABLE_ESP32_ESPIDF_CONSTS

};

/*
 * Register the built-in constants defined above.
 */
PH7_PRIVATE void PH7_RegisterExtensionsConstants(ph7_vm *pVm) {
  sxu32 n;
  /* 
   * TODO: why not pass constant value as a private data to a single callback?!
   * Goddamn copy/paste is everywhere
   * TODO: 
   */
  for (n = 0; n < SX_ARRAYSIZE(aBuiltIn); ++n) {
    ph7_create_constant(&(*pVm), aBuiltIn[n].zName, aBuiltIn[n].xExpand, &(*pVm));
  }
}
#endif

