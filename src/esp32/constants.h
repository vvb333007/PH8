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
 
/* Numeric constants expansion routines. These are shared among different constants
 *
 */
#pragma once

#include "ph7int.h"

#define Const( Value_ ) \
void esp32_const_int ## Value_ (ph7_value *, void *)

#define MinusConst( Value_ ) \
void esp32_const_mint ## Value_ (ph7_value *, void *)

void esp32_const_8n1( ph7_value *pVal, void *pUserData );

Const( 0 );
Const( 1 );
Const( 2 );
Const( 3 );
Const( 4 );
Const( 5 );
Const( 6 );
Const( 7 );
Const( 8 );
Const( 9 );
Const( 12 );
Const( 13 );
Const( 16 );
Const( 19 );
Const( 32 );
Const( 64 );
Const( 128 );
Const( 192 );
Const( 0x800001c );


MinusConst( 1 );

#undef Const
#undef MinusConst
