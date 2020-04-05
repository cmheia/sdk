#ifndef __LWIP_OPTS_H
#define __LWIP_OPTS_H

#include "wm_config.h"
#include "wm_mem.h"

#include <stdlib.h>

/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEM_LIBC_MALLOC                 1
#define mem_clib_free                   tls_mem_free
#define mem_clib_malloc                 tls_mem_alloc
#define mem_clib_calloc                 tls_mem_calloc

#define MEMP_MEM_MALLOC                 1
#define MEM_USE_POOLS                   0
#define MEMP_USE_CUSTOM_POOLS           0

#define LWIP_COMPAT_SOCKETS             2

#define LWIP_PROVIDE_ERRNO /* Make lwip/arch.h define the codes which are used */

#define LWIP_SOCKET                     TLS_CONFIG_SOCKET_STD
#define LWIP_NETCONN                    TLS_CONFIG_SOCKET_STD

#define MEM_SIZE                        30000

/**
 * TCP_WND: The size of a TCP window.  This must be at least
 * (2 * TCP_MSS) for things to work well
 */
#define TCP_WND                         (6 * TCP_MSS)

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#define TCP_MSS                         1024
//#define TCP_MSS                         (1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#define TCP_SND_BUF                     (15*TCP_MSS)

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
//#define TCP_SND_QUEUELEN                ((2 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_SND_QUEUELEN                30

#define MEMP_NUM_TCP_SEG                32

#ifndef TCP_MSL
#define TCP_MSL 1000UL /* The maximum segment lifetime in milliseconds */
#endif

/* Keepalive values, compliant with RFC 1122. Don't change this unless you know what you're doing */
#ifndef  TCP_KEEPIDLE_DEFAULT
#define  TCP_KEEPIDLE_DEFAULT     200000UL /* Default KEEPALIVE timer in milliseconds */
#endif

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#define TCPIP_MBOX_SIZE                 64

/**
 * LWIP_TCPIP_CORE_LOCKING: (EXPERIMENTAL!)
 * Don't use it if you're not an active lwIP project member
 */
#define LWIP_TCPIP_CORE_LOCKING         0

/**
 * LWIP_TCPIP_CORE_LOCKING_INPUT: (EXPERIMENTAL!)
 * Don't use it if you're not an active lwIP project member
 */
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0

#define LWIP_DHCP                       1

#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0

#define LWIP_NETIF_API                  1
/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 */
#define MEMP_NUM_NETCONN                8
#define LWIP_DEBUG
//#define LWIP_NOASSERT

#define TCP_DEBUG                       LWIP_DBG_OFF
//#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
//#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#define NAPT_DEBUG                      LWIP_DBG_OFF
#if (defined _DEBUG) && !(defined NDEBUG)
#define NETIF_DEBUG                     LWIP_DBG_ON
#define ETHARP_DEBUG                    LWIP_DBG_ON
#define DHCP_DEBUG                      LWIP_DBG_ON
#define DHCP6_DEBUG                     LWIP_DBG_ON
#define IP6_DEBUG                       LWIP_DBG_ON
#define ICMP_DEBUG                      LWIP_DBG_ON
#endif

#define LWIP_IGMP                       TLS_CONFIG_IGMP

extern int sys_rand(void);
#define LWIP_RAND                       sys_rand

#define LWIP_SO_RCVTIMEO                1

#define DHCP_DOES_ARP_CHECK             1

#define LWIP_IPV4                       TLS_CONFIG_IPV4
#define LWIP_IPV6                       TLS_CONFIG_IPV6
#define LWIP_DNS                        1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define TCP_LISTEN_BACKLOG              1
#define TCP_DEFAULT_LISTEN_BACKLOG      8
#define SO_REUSE                        1
#define LWIP_ND6_MAX_MULTICAST_SOLICIT  10
#define LWIP_RAW                        1

#define LWIP_HAVE_LOOPIF                1
#define ETHARP_SUPPORT_STATIC_ENTRIES   1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_TCP_KEEPALIVE              1

/** LWIP_NETCONN_SEM_PER_THREAD==1: Use one (thread-local) semaphore per
 * thread calling socket/netconn functions instead of allocating one
 * semaphore per netconn (and per select etc.)
 * ATTENTION: a thread-local semaphore for API calls is needed:
 * - LWIP_NETCONN_THREAD_SEM_GET() returning a sys_sem_t*
 * - LWIP_NETCONN_THREAD_SEM_ALLOC() creating the semaphore
 * - LWIP_NETCONN_THREAD_SEM_FREE() freeing the semaphore
 * The latter 2 can be invoked up by calling netconn_thread_init()/netconn_thread_cleanup().
 * Ports may call these for threads created with sys_thread_new().
 */
#define LWIP_NETCONN_SEM_PER_THREAD     0

/** LWIP_NETCONN_FULLDUPLEX==1: Enable code that allows reading from one thread,
 * writing from a 2nd thread and closing from a 3rd thread at the same time.
 * ATTENTION: This is currently really alpha! Some requirements:
 * - LWIP_NETCONN_SEM_PER_THREAD==1 is required to use one socket/netconn from
 *   multiple threads at once
 * - sys_mbox_free() has to unblock receive tasks waiting on recvmbox/acceptmbox
 *   and prevent a task pending on this during/after deletion
 */
#define LWIP_NETCONN_FULLDUPLEX         0

#define LWIP_HOOK_FILENAME "arch/lwip_hooks.h"

#ifdef TLS_CONFIG_DHCP_OPTION60
#define LWIP_HOOK_DHCP_APPEND_OPTIONS   hook_dhcp4_append_options
#endif

#if (defined _DEBUG) && !(defined NDEBUG)
#define LWIP_ARP_FILTER_NETIF           1
#endif
#define LWIP_ARP_FILTER_NETIF_FN        hook_arp_filter_netif

/* GCC 9.2.1 */
#define LWIP_SOCKET_SELECT              1
#define LWIP_TIMEVAL_PRIVATE            0
#define IN_ADDR_T_DEFINED

#endif /* end of __LWIP_OPTS_H */
