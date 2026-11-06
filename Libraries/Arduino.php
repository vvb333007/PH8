<?php

/* Arduino ADC native constants */

const ADC_0db = 0;
const ADC_2_5db = 1;
const ADC_6db = 2;
const ADC_11db = 3;
const ADC_ATTENDB_MAX = 4;

/* Arduino GPIO native constants */

const LOW = 0;
const HIGH = 1;

const INPUT = 1;
const OUTPUT = 3;
const PULLUP = 4;
const INPUT_PULLUP = 5;
const PULLDOWN = 8;
const INPUT_PULLDOWN = 9;
const OPEN_DRAIN = 16;
const OUTPUT_OPEN_DRAIN = 19;
const ANALOG = 192;
const DISABLED = 0;
const RISING = 1;
const FALLING = 2;
const CHANGE = 3;
const ONLOW = 4;
const ONHIGH = 5;
const ONLOW_WE = 12;
const ONHIGH_WE = 13;

  /* Arduino SPI
   */
if (CONFIG_IDF_TARGET_ESP32) {
const FSPI = 1;
const HSPI = 2;
const VSPI = 3;
} else {
const FSPI = 0;
const HSPI = 1;
}

const SPI_CLOCK_DIV2 = 2;
const SPI_CLOCK_DIV4 = 4;
const SPI_CLOCK_DIV8 = 8;
const SPI_CLOCK_DIV16 = 16;
const SPI_CLOCK_DIV32 = 32;
const SPI_CLOCK_DIV64 = 64;
const SPI_CLOCK_DIV128 = 128;

const SPI_MODE0 = 0;
const SPI_MODE1 = 1;
const SPI_MODE2 = 2;
const SPI_MODE3 = 3;

const SPI_SS0 = 0;
const SPI_SS1 = 1;
const SPI_SS2 = 2;
const SPI_SS_MASK_ALL = 7;

const SPI_LSBFIRST = 0;
const SPI_MSBFIRST = 1;

  /* Arduino CPU
   *
   */
const APB_BEFORE_CHANGE = 0;
const APB_AFTER_CHANGE = 1;

const RGB_BRIGHTNESS = 64;
const RGB_BUILTIN_LED_COLOR_ORDER = 5;  // default WS2812B color order, GRB
const LED_COLOR_ORDER_RGB = 0;
const LED_COLOR_ORDER_BGR = 1;
const LED_COLOR_ORDER_BRG = 2;
const LED_COLOR_ORDER_RBG = 3;
const LED_COLOR_ORDER_GBR = 4;
const LED_COLOR_ORDER_GRB = 5;

?>
