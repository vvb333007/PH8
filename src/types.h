/*
 * PH7-ESP32 — embeddable PHP bytecode compiler & virtual machine,
 * ported and maintained for ESP32/FreeRTOS, Linux and Windows(Cygwin).
 *
 * Copyright (C) 2011-2018 Symisc Systems <http://ph7.symisc.net/>
 * Copyright (C) 2026-present Viacheslav Logunov <vvb333007@gmail.com>
 *
 * Licensed under the Symisc Public License (modified BSD with mandatory
 * source disclosure, clause 3); commercial licensing available from
 * Symisc Systems. Full terms: see LICENSE. Contacts: see CONTACTS.
 */


#if !defined(SYMISC_STD_TYPES)
#define SYMISC_STD_TYPES

typedef signed char sxi8;         /* signed char */
typedef unsigned char sxu8;       /* unsigned char */

typedef signed short int sxi16;   /* 16 bits(2 bytes) signed integer */
typedef unsigned short int sxu16; /* 16 bits(2 bytes) unsigned integer */

typedef int sxi32;                /* 32 bits(4 bytes) integer */
typedef unsigned int sxu32;       /* 32 bits(4 bytes) unsigned integer */

typedef signed long long int sxi64;   /* 64 bits(8 bytes) signed int64 */
typedef unsigned long long int sxu64; /* 64 bits(8 bytes) unsigned int64 */

typedef long sxptr;
typedef unsigned long sxuptr;

typedef long sxlong;
typedef unsigned long sxulong;

typedef sxi32 sxofft;
typedef sxi64 sxofft64;

typedef long double sxlongreal;
typedef double sxreal;

#define SXI8_HIGH 0x7F
#define SXU8_HIGH 0xFF
#define SXI16_HIGH 0x7FFF
#define SXU16_HIGH 0xFFFF
#define SXI32_HIGH 0x7FFFFFFF
#define SXU32_HIGH 0xFFFFFFFF
#define SXI64_HIGH 0x7FFFFFFFFFFFFFFF
#define SXU64_HIGH 0xFFFFFFFFFFFFFFFF

#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#endif
