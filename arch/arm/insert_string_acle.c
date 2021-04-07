/* insert_string_acle.c -- insert_string integer hash variant using ACLE's CRC instructions
 *
 * Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 */

#ifdef ARM_ACLE
#ifndef _MSC_VER
#  include <arm_acle.h>
#endif
#include "../../zbuild.h"
#include "../../deflate.h"

#if defined(__clang__)
/* CRC32 intrinsics are #ifdef'ed out of arm_acle.h unless we build with an
 * armv8 target, which is incompatible with ThinLTO optimizations on Android.
 * (Namely, mixing and matching different module-level targets makes ThinLTO
 * warn, and Android defaults to armv7-a. This restriction does not apply to
 * function-level `target`s, however.)
 *
 * Since we only need four crc intrinsics, and since clang's implementation of
 * those are just wrappers around compiler builtins, it's simplest to #define
 * those builtins directly. If this #define list grows too much (or we depend on
 * an intrinsic that isn't a trivial wrapper), we may have to find a better way
 * to go about this.
 *
 * NOTE: clang currently complains that "'+soft-float-abi' is not a recognized
 * feature for this target (ignoring feature)." This appears to be a harmless
 * bug in clang.
 */
#define __crc32b __builtin_arm_crc32b
#define __crc32d __builtin_arm_crc32d
#define __crc32w __builtin_arm_crc32w
#define __crc32cw __builtin_arm_crc32cw

#if defined(__aarch64__)
#define TARGET_ARMV8_WITH_CRC __attribute__((target("crc")))
#else  // !defined(__aarch64__)
#define TARGET_ARMV8_WITH_CRC __attribute__((target("armv8-a,crc")))
#endif  // defined(__aarch64__)

#elif defined(__GNUC__)
/* For GCC, we are setting CRC extensions at module level, so ThinLTO is not
 * allowed. We can just include arm_acle.h.
 */
#include <arm_acle.h>
#define TARGET_ARMV8_WITH_CRC
#else  // !defined(__GNUC__) && !defined(_aarch64__)
#error ARM CRC32 SIMD extensions only supported for Clang and GCC
#endif

#define HASH_CALC(s, h, val) \
    h = __crc32w(0, val)

#define HASH_CALC_VAR       h
#define HASH_CALC_VAR_INIT  uint32_t h = 0

#define UPDATE_HASH         update_hash_acle
#define INSERT_STRING       insert_string_acle
#define QUICK_INSERT_STRING quick_insert_string_acle

#include "../../insert_string_tpl.h"
#endif
