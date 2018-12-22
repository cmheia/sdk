#include "wm_include.h"
#include "lwip/inet.h"
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "ping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OWNER_PING_ID 12345
#define PING_DATA_LEN 32
#define PACKET_SIZE 64
#define MAX_NO_PACKETS 3
#define ICMP_HEAD_LEN 8

#define PING_TEST_START 0x1

#define TASK_PING_PRIO 35
#define TASK_PING_STK_SIZE 256
#define PING_QUEUE_SIZE 4
#define PING_STOP_TIMER_DELAY (2 * HZ)
#define PING_ABORT_TIMER_DELAY (1 * HZ)

#if TLS_CONFIG_WIFI_PING_TEST
static bool              ping_task_running = FALSE;
static OS_STK            TaskPingStk[TASK_PING_STK_SIZE];
static tls_os_queue_t *  ping_msg_queue = NULL;
static tls_os_timer_t *  ping_test_stop_timer;
static tls_os_timer_t *  ping_test_abort_timer;
static u8                ping_test_running = FALSE;
static u8                ping_test_abort   = FALSE;
static struct ping_param g_ping_para;
static u32               received_cnt = 0;
static u32               send_cnt     = 0;

static u16 ping_test_chksum(u16 *addr, int len)
{
    int  nleft  = len;
    int  sum    = 0;
    u16 *w      = addr;
    u16  answer = 0;

    /*把ICMP报头二进制数据以2字节为单位累加起来*/
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
    if (nleft == 1) {
        *(u8 *) (&answer) = *(u8 *) w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

/*设置ICMP报头*/
static int ping_test_pack(int pack_no, char *sendpacket, sa_family_t ss_family)
{
    int  packsize;
    u32 *tval;

    if (AF_INET6 == ss_family) {
        struct icmp6_echo_hdr *iecho = (struct icmp6_echo_hdr *) sendpacket;

        iecho->type   = ICMP6_TYPE_EREQ;
        iecho->code   = 0;
        iecho->chksum = 0;
        iecho->seqno  = pack_no;
        iecho->id     = OWNER_PING_ID;
        tval          = (u32 *) &iecho[1]; /* icmp data */
        *tval         = tls_os_get_time(); /*记录发送时间*/
        /* 填充剩下的28个字符 */
        memset(tval + 1, 0xff, PING_DATA_LEN - sizeof(u32));
        packsize = sizeof(struct icmp6_echo_hdr) + PING_DATA_LEN;
    } else {
        struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *) sendpacket;

        ICMPH_TYPE_SET(iecho, ICMP_ECHO);
        ICMPH_CODE_SET(iecho, 0);
        iecho->chksum = 0;
        iecho->seqno  = pack_no;
        iecho->id     = OWNER_PING_ID;
        tval          = (u32 *) &iecho[1]; /* icmp data */
        *tval         = tls_os_get_time(); /*记录发送时间*/
        /* 填充剩下的28个字符 */
        memset(tval + 1, 0xff, PING_DATA_LEN - sizeof(u32));
        packsize      = sizeof(struct icmp_echo_hdr) + PING_DATA_LEN;
        iecho->chksum = ping_test_chksum((u16 *) iecho, packsize);
    }
    return packsize;
}

/*剥去ICMP报头*/
static void ping_test_unpack(char *                   buf,
                             int                      len,
                             u32                      tvrecv,
                             struct sockaddr_storage *from,
                             struct sockaddr_storage *dest_addr)
{
    int  iphdrlen;
    u32 *tvsend;
    u32  rtt;

    if (from->ss_family != dest_addr->ss_family) {
        return;
    }
    if (AF_INET6 == from->ss_family) {
        struct ip6_hdr *       ip;
        struct icmp6_echo_hdr *iecho;

        ip       = (struct ip6_hdr *) buf;
        iphdrlen = IP6H_PLEN(ip);
        iecho    = (struct icmp6_echo_hdr *) (buf + iphdrlen);

        len -= iphdrlen;
        if (len < sizeof(struct icmp6_echo_hdr)) {
            printf("ICMP packets's length is less than %d\r\n",
                   sizeof(struct icmp6_echo_hdr));
            return;
        }

        if ((ICMP6_TYPE_EREP == iecho->type) && (iecho->id == OWNER_PING_ID)) {
            tvsend = (u32 *) (iecho + 1);
            rtt    = (tvrecv - (*tvsend)) * (1000 / HZ);
            len -= sizeof(struct icmp6_echo_hdr);
            if (0 == rtt) {
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt<%u ms\n",
                       len,
                       inet6_ntoa(((struct sockaddr_in6 *) from)->sin6_addr),
                       iecho->seqno,
                       IP6H_HOPLIM(ip),
                       1000 / HZ);
            } else {
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%u ms\n",
                       len,
                       inet6_ntoa(((struct sockaddr_in6 *) from)->sin6_addr),
                       iecho->seqno,
                       IP6H_HOPLIM(ip),
                       rtt);
            }
            received_cnt++;
        }
    } else {
        struct ip_hdr *       ip;
        struct icmp_echo_hdr *iecho;

        ip       = (struct ip_hdr *) buf;
        iphdrlen = IPH_HL(ip) * 4;
        iecho    = (struct icmp_echo_hdr *) (buf + iphdrlen);

        len -= iphdrlen;
        if (len < sizeof(struct icmp_echo_hdr)) {
            printf("ICMP packets's length is less than %d\r\n",
                   sizeof(struct icmp_echo_hdr));
            return;
        }

        if ((ICMP_ER == ICMPH_TYPE(iecho)) && (iecho->id == OWNER_PING_ID)) {
            tvsend = (u32 *) (iecho + 1);
            rtt    = (tvrecv - (*tvsend)) * (1000 / HZ);
            len -= sizeof(struct icmp_echo_hdr);
            if (0 == rtt) {
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt<%u ms\n",
                       len,
                       inet_ntoa(((struct sockaddr_in *) from)->sin_addr),
                       iecho->seqno,
                       IPH_TTL(ip),
                       1000 / HZ);
            } else {
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%u ms\n",
                       len,
                       inet_ntoa(((struct sockaddr_in *) from)->sin_addr),
                       iecho->seqno,
                       IPH_TTL(ip),
                       rtt);
            }
            received_cnt++;
        }
    }
}

static sa_family_t _inet_pton(const char *host, struct sockaddr_storage *to)
{
    if (inet_pton(AF_INET, host, &((struct sockaddr_in *) to)->sin_addr)) {
        to->ss_family = AF_INET;
        to->s2_len    = sizeof(struct sockaddr_in);
        return AF_INET;
    } else if (inet_pton(
                   AF_INET6, host, &((struct sockaddr_in6 *) to)->sin6_addr)) {
        to->ss_family = AF_INET6;
        to->s2_len    = sizeof(struct sockaddr_in6);
        return AF_INET6;
    }
    return AF_UNSPEC;
}

static const char *
_inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (AF_INET6 == af) {
        return inet_ntop(
            af, &((struct sockaddr_in6 *) src)->sin6_addr, dst, size);
    } else if (AF_INET == af) {
        return inet_ntop(
            af, &((struct sockaddr_in *) src)->sin_addr, dst, size);
    } else {
        return NULL;
    }
}

static int ping_test_init(struct sockaddr_storage *dest_addr)
{
    int         sockfd   = -1;
    char *      hostname = NULL;
    sa_family_t domain;

    send_cnt     = 0;
    received_cnt = 0;

    hostname = g_ping_para.host;
    /*判断是主机名还是ip地址*/
    domain = _inet_pton(hostname, dest_addr);
    if ((AF_INET == domain) || (AF_INET6 == domain)) {
        hostname = NULL;
        if (AF_INET6 == domain) {
            sockfd = socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
        } else {
            sockfd = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
        }
        if (sockfd < 0) {
            printf("create socket failed.\r\n");
            return -1;
        }
    } else {
        struct addrinfo  hints;
        struct addrinfo *addr_list, *cur;

        /* Do name resolution with both IPv6 and IPv4 */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC; /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_RAW;
        hints.ai_protocol = IPPROTO_ICMP;

        if (getaddrinfo(hostname, NULL, &hints, &addr_list)) {
            return -1;
        }

        /* getaddrinfo() returns a list of address structures. */
        for (cur = addr_list; cur; cur = cur->ai_next) {
            if (AF_INET == cur->ai_family) {
                struct sockaddr_in *addr = (struct sockaddr_in *) dest_addr;
                dest_addr->s2_len        = cur->ai_addrlen;
                dest_addr->ss_family     = cur->ai_family;
                memcpy(addr, cur->ai_addr, sizeof(struct sockaddr_in));
                break;
            } else if (AF_INET6 == cur->ai_family) {
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *) dest_addr;
                dest_addr->s2_len         = cur->ai_addrlen;
                dest_addr->ss_family      = cur->ai_family;
                memcpy(addr, cur->ai_addr, sizeof(struct sockaddr_in6));
                break;
            }
        }
        if (cur) {
            domain = cur->ai_family;
            if (AF_INET6 == cur->ai_family) {
                sockfd = socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
            } else {
                sockfd = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
            }
            freeaddrinfo(addr_list); /* No longer needed */
            if (sockfd < 0) {
                printf("create socket failed.\r\n");
                return -1;
            }
        } else {
            printf("Ping request could not find host \"%s\"\r\n"
                   "    Please check the name and try again.",
                   hostname);
            freeaddrinfo(addr_list); /* No longer needed */
            return -1;
        }
    }

    if (hostname) { /*是主机名*/
        char dest_name[IP6ADDR_STRLEN_MAX + 1];
        memset(dest_name, 0, sizeof(dest_name));
        printf("\r\nPING %s(%s): %d bytes data in ICMP packets.\r\n",
               hostname,
               _inet_ntop(domain, dest_addr, dest_name, sizeof(dest_name)),
               PING_DATA_LEN);
    } else { /*是ip地址*/
        printf("\r\nPING %s: %d bytes data in ICMP packets.\r\n",
               g_ping_para.host,
               PING_DATA_LEN);
    }

    return sockfd;
}

static void ping_test_stat(void)
{
    printf("\n--------------------PING statistics-------------------\n");
    printf("%u packets transmitted, %u received , %u(%.3g%%) lost.\n",
           send_cnt,
           received_cnt,
           send_cnt >= received_cnt ? send_cnt - received_cnt : 0,
           send_cnt >= received_cnt
               ? ((double) (send_cnt - received_cnt)) / send_cnt * 100
               : 0);
}

static void ping_test_recv(int sockfd, struct sockaddr_storage *dest_addr)
{
    fd_set         read_set;
    struct timeval tv;
    int            ret;

    for (;;) {
        FD_ZERO(&read_set);
        FD_SET(sockfd, &read_set);
        tv.tv_sec  = 0;
        tv.tv_usec = 1;

        ret = select(sockfd + 1, &read_set, NULL, NULL, &tv);
        if (ret > 0) {
            if (FD_ISSET(sockfd, &read_set)) {
                u32                     tvrecv;
                char                    recvpacket[PACKET_SIZE];
                struct sockaddr_storage from;
                socklen_t               fromlen = sizeof(from);
                memset(recvpacket, 0, PACKET_SIZE);
                ret = recvfrom(sockfd,
                               recvpacket,
                               sizeof(recvpacket),
                               0,
                               (struct sockaddr *) &from,
                               &fromlen);
                if (ret < 0) {
                    // printf("%d: recvfrom error\r\n", received_cnt + 1);
                    break;
                }

                tvrecv = tls_os_get_time(); /*记录接收时间*/
                ping_test_unpack(recvpacket, ret, tvrecv, &from, dest_addr);

                FD_CLR(sockfd, &read_set);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

static void ping_test_send(int sockfd, struct sockaddr_storage *dest_addr)
{
    int  packetsize;
    char sendpacket[PACKET_SIZE];

    if ((0 != g_ping_para.cnt) && (send_cnt >= g_ping_para.cnt)) {
        return;
    }

    memset(sendpacket, 0, PACKET_SIZE);
    packetsize = ping_test_pack(send_cnt, sendpacket, dest_addr->ss_family);
    if (sendto(sockfd,
               sendpacket,
               packetsize,
               0,
               (struct sockaddr *) dest_addr,
               sizeof(*dest_addr))
        < 0) {
        // printf("%d: send icmp echo failed\r\n", send_cnt + 1);
        return;
    }
    send_cnt++;

    if ((0 != g_ping_para.cnt) && (send_cnt >= g_ping_para.cnt)) {
        tls_os_timer_start(ping_test_stop_timer);
    }
}

static void ping_test_run(void)
{
    int                     sockfd;
    struct sockaddr_storage dest_addr;
    u32                     lastTime = 0;
    u32                     curTime  = 0;

    memset(&dest_addr, 0, sizeof(dest_addr));
    sockfd = ping_test_init(&dest_addr);
    if (sockfd < 0) {
        return;
    }

    ping_test_abort   = FALSE;
    ping_test_running = TRUE;

    for (;;) {
        if (!ping_test_running) {
            break;
        }

        if (!ping_test_abort) {
            curTime = tls_os_get_time();
            if ((curTime - lastTime) >= (g_ping_para.interval / (1000 / HZ))) {
                ping_test_send(sockfd, &dest_addr);
                lastTime = tls_os_get_time();
            }
        }
        ping_test_recv(sockfd, &dest_addr);
    }

    tls_os_timer_stop(ping_test_stop_timer);
    closesocket(sockfd);

    ping_test_stat();
}

static void ping_test_task(void *data)
{
    void *msg;

    for (;;) {
        tls_os_queue_receive(ping_msg_queue, (void **) &msg, 0, 0);
        tls_os_time_delay(2);
        switch ((u32) msg) {
        case PING_TEST_START:
            ping_test_run();
            break;

        default:
            break;
        }
    }
}

static void ping_test_stop_timeout(void *ptmr, void *parg)
{
    ping_test_stop();
}

static void ping_test_abort_timeout(void *ptmr, void *parg)
{
    ping_test_running = FALSE;
}

void ping_test_create_task(void)
{
    if (ping_task_running) {
        return;
    }

    tls_os_task_create(NULL,
                       NULL,
                       ping_test_task,
                       (void *) 0,
                       (void *) TaskPingStk,
                       TASK_PING_STK_SIZE * sizeof(u32),
                       TASK_PING_PRIO,
                       0);

    ping_task_running = TRUE;

    tls_os_queue_create(&ping_msg_queue, PING_QUEUE_SIZE);

    tls_os_timer_create(&ping_test_stop_timer,
                        ping_test_stop_timeout,
                        NULL,
                        PING_STOP_TIMER_DELAY,
                        FALSE,
                        NULL);

    tls_os_timer_create(&ping_test_abort_timer,
                        ping_test_abort_timeout,
                        NULL,
                        PING_ABORT_TIMER_DELAY,
                        FALSE,
                        NULL);
}

void ping_test_start(struct ping_param *para)
{
    if (ping_test_running) {
        return;
    }

    memcpy(&g_ping_para, para, sizeof(struct ping_param));
    tls_os_queue_send(ping_msg_queue, (void *) PING_TEST_START, 0);
}

void ping_test_stop(void)
{
    ping_test_abort = TRUE;
    tls_os_timer_start(ping_test_abort_timer);
}
#endif

