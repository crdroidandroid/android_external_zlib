/* crc32_acle.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2006, 2010, 2011, 2012 Mark Adler
 * Copyright (C) 2016 Yang Zhang
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
*/

#ifdef ARM_ACLE
#ifdef _MSC_VER
#  include <intrin.h>
#else
#  include <arm_acle.h>
#endif
#include "../../zbuild.h"

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
#define __crc32h __builtin_arm_crc32h

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

TARGET_ARMV8_WITH_CRC
Z_INTERNAL uint32_t crc32_acle(uint32_t crc, const uint8_t *buf, size_t len) {
    Z_REGISTER uint32_t c;
    Z_REGISTER const uint16_t *buf2;
    Z_REGISTER const uint32_t *buf4;

    c = ~crc;
    if (len && ((ptrdiff_t)buf & 1)) {
        c = __crc32b(c, *buf++);
        len--;
    }

    if ((len >= sizeof(uint16_t)) && ((ptrdiff_t)buf & sizeof(uint16_t))) {
        buf2 = (const uint16_t *) buf;
        c = __crc32h(c, *buf2++);
        len -= sizeof(uint16_t);
        buf4 = (const uint32_t *) buf2;
    } else {
        buf4 = (const uint32_t *) buf;
    }

#if defined(__aarch64__) || defined(_M_ARM64)
    if ((len >= sizeof(uint32_t)) && ((ptrdiff_t)buf & sizeof(uint32_t))) {
        c = __crc32w(c, *buf4++);
        len -= sizeof(uint32_t);
    }

    if (len == 0) {
        c = ~c;
        return c;
    }

    const uint64_t *buf8 = (const uint64_t *) buf4;

    while (len >= sizeof(uint64_t)) {
        c = __crc32d(c, *buf8++);
        len -= sizeof(uint64_t);
    }

    if (len >= sizeof(uint32_t)) {
        buf4 = (const uint32_t *) buf8;
        c = __crc32w(c, *buf4++);
        len -= sizeof(uint32_t);
        buf2 = (const uint16_t *) buf4;
    } else {
        buf2 = (const uint16_t *) buf8;
    }

    if (len >= sizeof(uint16_t)) {
        c = __crc32h(c, *buf2++);
        len -= sizeof(uint16_t);
    }

    buf = (const unsigned char *) buf2;
#else /* __aarch64__ */

    if (len == 0) {
        c = ~c;
        return c;
    }

    while (len >= sizeof(uint32_t)) {
        c = __crc32w(c, *buf4++);
        len -= sizeof(uint32_t);
    }

    if (len >= sizeof(uint16_t)) {
        buf2 = (const uint16_t *) buf4;
        c = __crc32h(c, *buf2++);
        len -= sizeof(uint16_t);
        buf = (const unsigned char *) buf2;
    } else {
        buf = (const unsigned char *) buf4;
    }
#endif /* __aarch64__ */

    if (len) {
        c = __crc32b(c, *buf);
    }

    c = ~c;
    return c;
}
#endif
