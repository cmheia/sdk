
#ifndef CC_H_INCLUDED
#define CC_H_INCLUDED

#include <stdio.h>

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#define LWIP_NO_STDINT_H 1
#include "wm_type_def.h"

/* Display name of types */
#define U16_F           "hu"
#define S16_F           "hd"
#define X16_F           "hx"
#define U32_F           "u"
#define S32_F           "d"
#define X32_F           "x"

/* Compiler hints for packing lwip's structures */
#if defined(__CC_ARM)
    /* Setup PACKing macros for MDK Tools */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#elif defined (__ICCARM__)
    /* Setup PACKing macros for EWARM Tools */
#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#elif defined (__GNUC__)
    /* Setup PACKing macros for GCC Tools */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#else
#error "This compiler does not support."
#endif

/* define LWIP_COMPAT_MUTEX
    to let sys.h use binary semaphores instead of mutexes - as before in 1.3.2
    Refer CHANGELOG
*/
#define  LWIP_COMPAT_MUTEX  1

#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   printf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); \
    } while(0)
#else
#define LWIP_PLATFORM_ASSERT(x)
#endif

#define LWIP_PLATFORM_DIAG(x)                                                 \
    do {                                                                      \
        extern bool __g_lwip_debug_print;                                     \
        if (__g_lwip_debug_print) {                                           \
            printf x;                                                         \
        }                                                                     \
    } while (0)

#define LWIP_ERRNO_STDINCLUDE

#endif /* CC_H_INCLUDED */
