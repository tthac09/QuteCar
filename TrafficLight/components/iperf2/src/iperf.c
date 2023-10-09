/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: implementation for iperf2.
 * Author: GaoQingshui
 * Create: 2020-02-26
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS OS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

/* The main difference between this implementation and opensource are following:
 * 1, for UDP server, do NOT ouput jitter and lost ratio every interval seconds,
 *    as it is very high CPU consumption to calculate jitter for every packet;
 * 2, only support normal mode, neither bidirectional and tradeoff mode is supported;
 * 3, only support one connection (-P option not supported);
 * 4, TCP Max Segment Size is not supported (-m option);
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <securec.h>
#include "lwip/sockets.h"
#include "los_task.h"
#include "los_mux.h"
#include "hi_task.h"
#include "hi_at.h"

#ifdef CUSTOM_AT_COMMAND
#define IPERF_PRINT(x) do { hi_at_printf x; } while (0)
#else
#define IPERF_PRINT(x) do { printf x; } while (0)
#endif

/* some platform need to feed wdg during iperf testing, such as Hi3861 */
#define IPERF_FEED_WDG
#ifdef IPERF_FEED_WDG
#include "hi_cpu.h"
#endif

#define IPERF_OK   0
#define IPERF_FAIL (-1)
#define IPERF_INVALID_SOCKET (-1)

#define IPERF_DEFAULT_PORT 5001
#define IPERF_DEFAULT_INTERVAL 1
#define IPERF_DEFAULT_TIME 30
#define IPERF_DEFAULT_MAX_TIME (86400 * 100) /* 100 Days */
#define IPERF_DEFAULT_UDP_BW (1024 * 1024) /* 1Mbps */
#define IPERF_DEFAULT_UDP_BUFLEN   1470
#define IPERF_DEFAULT_TCP_BUFLEN   0x2000 /* 8K */
#define IPERF_DEFAULT_RX_TIMEOUT 10 /* 10sec */
#define IPERF_DEFAULT_TX_TIMEOUT 10 /* 10sec */

#define IPERF_MASK_CLIENT (1U << 0)
#define IPERF_MASK_SERVER (1U << 1)
#define IPERF_MASK_UDP    (1U << 2)
#define IPERF_MASK_TCP    (1U << 3)
#define IPERF_MASK_IPV6   (1U << 4)

#define IPERF_TRAFFIC_NAME "iperf_traffic"
#define IPERF_TRAFFIC_CLIENT_PRIORITY 20
#define IPERF_TRAFFIC_SERVER_PRIORITY 3
#define IPERF_TRAFFIC_STACK_SIZE 0x1000 /* Must be greater than or equals 4K */
#define IPERF_REPORTER_NAME "iperf_reporter"
#define IPERF_REPORTER_PRIORITY 2
#define IPERF_REPORTER_STACK_SIZE 0x800 /* Must be greater than or equals 2K */

#define IPERF_HEADER_VERSION1 0x80000000
#define IPERF_THOUSAND  1000       /* 1000 */
#define IPERF_MILLION   1000000    /* 1000 * 1000 */
#define IPERF_BILLION   1000000000 /* 1000 * 1000 * 1000 */
#define IPERF_KILO      0x400      /* 1024 */
#define IPERF_MEGA      0x100000   /* 1024 * 1024 */
#define IPERF_GIGA      0x40000000 /* 1024 * 1024 * 1024 */
#define IPERF_BITS_PER_BYTE 8

#define IPERF_UDP_FIN_MAX_RETRIES 10 /* UDP FIN or FINACK max retries */
#define IPERF_UDP_DELAY (IPERF_MILLION / LOSCFG_BASE_CORE_TICK_PER_SECOND)
#define IPERF_UDP_FIN_TMO 250000 /* 250ms: select timeout for UDP FIN */
#define IPERF_UDP_FINACK_TMO 1 /* 1s: select timeout for UDP FINACK */


typedef struct {
    union {
        struct in_addr srcIP4;
        struct in6_addr srcIP6;
    } u1;
    union {
        struct in_addr dstIP4;
        struct in6_addr dstIP6;
    } u2;
    uint16_t port;
    uint8_t  mask;
    uint8_t  tos;
    uint32_t interval;
    uint32_t time;
    uint64_t bandwidth;
    uint64_t amount;
    uint64_t total;
    uint8_t *buffer;
    uint32_t bufLen;
} IperfParams;

typedef struct {
    int32_t id;
    uint32_t sec;
    uint32_t usec;
} IperfUdpHdr;

/* from iperf opensource, preclude some additional elements */
typedef struct {
    int32_t flags;
    int32_t totalLenH;
    int32_t totalLenL;
    int32_t endSec;
    int32_t endUsec;
    int32_t errCnt;
    int32_t oooCnt;
    int32_t datagrams;
    int32_t jitterSec;
    int32_t jitterUsec;
} IperfUdpServerHdr;

typedef struct {
    int32_t flags;
    int32_t numThreads;
    int32_t port;
    int32_t bufLen;
    int32_t winBand;
    int32_t amount;
} IperfUdpClientHdr;

typedef struct {
    BOOL isRunning;
    BOOL isFinish; /* iperf stopped normally */
    BOOL isKilled; /* iperf killed by user */
    int32_t listenSock;
    int32_t trafficSock;
    int32_t udpPktID;
    uint32_t mutex;
    struct timeval start;
    struct timeval end;
    IperfParams param;
} IperfContext;

static IperfContext *g_iperfContext = NULL;
static uint32_t g_iperfMutex = 0xFFFFFFFF;

#define IPERF_IS_SERVER(mask) (((mask) & IPERF_MASK_SERVER) == IPERF_MASK_SERVER)
#define IPERF_IS_CLIENT(mask) (((mask) & IPERF_MASK_CLIENT) == IPERF_MASK_CLIENT)
#define IPERF_IS_UDP(mask)    (((mask) & IPERF_MASK_UDP)    == IPERF_MASK_UDP)
#define IPERF_IS_TCP(mask)    (((mask) & IPERF_MASK_TCP)    == IPERF_MASK_TCP)
#define IPERF_IS_IPV6(mask)   (((mask) & IPERF_MASK_IPV6)   == IPERF_MASK_IPV6)

#define ADDR6_INIT(domain, port, address, sockaddr)  do { \
    (sockaddr).sin6_family = (domain);  \
    (sockaddr).sin6_port   = (port);  \
    (sockaddr).sin6_addr   = (address);  \
} while (0)

#define ADDR4_INIT(domain, port, address, sockaddr)  do { \
    (sockaddr).sin_family = (domain);  \
    (sockaddr).sin_port   = (port);  \
    (sockaddr).sin_addr   = (address);  \
} while (0)

#ifdef IPERF_FEED_WDG
static void IperfFeedWdg(void)
{
    hi_cpup_load_check_proc(hi_task_get_current_id(), LOAD_SLEEP_TIME_DEFAULT);
}
#endif /* IPERF_FEED_WDG */

static BOOL IsIperfRunning(void)
{
    if (g_iperfContext != NULL && g_iperfContext->isRunning == TRUE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int64_t TimeUsec(struct timeval end, struct timeval start)
{
    const int32_t maxGap = 2000; /* 2000: max delta */

    int32_t delta;
    if (end.tv_sec - start.tv_sec > maxGap) {
        delta = maxGap;
    } else if (end.tv_sec - start.tv_sec < -maxGap) {
        delta = -maxGap;
    } else {
        delta = end.tv_sec - start.tv_sec;
    }

    return (delta * IPERF_MILLION) + (end.tv_usec - start.tv_usec);
}

static float TimeSec(struct timeval end, struct timeval start)
{
    if (end.tv_sec - start.tv_sec < -IPERF_DEFAULT_MAX_TIME) {
        return -IPERF_DEFAULT_MAX_TIME;
    } else if (end.tv_sec - start.tv_sec > IPERF_DEFAULT_MAX_TIME) {
        return IPERF_DEFAULT_MAX_TIME;
    } else {
        return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / (float)IPERF_MILLION;
    }
}

static void IperfDelay(uint32_t usecs)
{
    uint32_t ticks = usecs / IPERF_UDP_DELAY;
    if (ticks) {
        LOS_TaskDelay(ticks);
    } else {
        LOS_TaskDelay(1); /* 1: atleast one tick */
    }
}

static void PrintBandwidth(uint64_t amount, float start, float end)
{
    int unit;
    char amountSuffix = 'K';
    char rateSuffix = 'K';
    float transfer;
    float interval = end - start;

    if (amount > IPERF_MEGA) {
        amountSuffix = 'M';
        transfer = amount / (float)IPERF_MEGA;
    } else {
        transfer = amount / (float)IPERF_KILO;
    }

    if (((amount * IPERF_BITS_PER_BYTE) / interval) >= (float)IPERF_MILLION) {
        unit = IPERF_MILLION; /* Mbps */
        rateSuffix = 'M';
    } else {
        unit = IPERF_THOUSAND; /* Kbps */
    }

    /* do NOT use double precision, single precision is enough */
    IPERF_PRINT(("%4.1f-%4.1f sec    %6.2f %cBytes    %.2f %cbits/sec\r\n", start, end,
                transfer, amountSuffix, ((float)amount * 8) / interval / (float)unit, rateSuffix));
}

static void IperfReporterProcess(IperfContext *context,
                                 uint32_t *passed, uint64_t *lastTotal)
{
    PrintBandwidth(context->param.total - *lastTotal, *passed,
                   (*passed + context->param.interval));

    *passed += context->param.interval;
    *lastTotal = context->param.total;
    if ((context->param.time != 0) && (*passed >= context->param.time)) {
        context->isFinish = TRUE;
    }
}

/* iperf reporter task, output testing result every interval seconds (-i option);
 * two stop condition:
 * 1, iperf killed by user (-k ioption, isKilled is true);
 * 2, iperf client stop the testing (isFinish is true);
 * 3, time reached (-t option, setting -t is not recommened in server mode);
 */
static void IperfReporterEntry(void)
{
    IperfContext *context = NULL;
    uint64_t lastTotal = 0;
    uint32_t passed = 0;
    uint32_t count = 0;

    if (LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return;
    }
    context = g_iperfContext;
    if (context == NULL) {
        LOS_MuxPost(g_iperfMutex);
        return;
    }

    IPERF_PRINT(("+IPERF:\r\n%-16s    %-16s    %-16s\r\n", "Interval", "Transfer", "Bandwidth"));

    while ((context->isFinish == FALSE) && (context->isKilled == FALSE)) {
        LOS_MuxPost(g_iperfMutex);
        LOS_TaskDelay(LOSCFG_BASE_CORE_TICK_PER_SECOND / 5); /* 5: schedule every 200ms */
        if (LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER) != LOS_OK) {
            break;
        }

        context = g_iperfContext;
        if (context == NULL) {
            LOS_MuxPost(g_iperfMutex);
            break;
        }
        if (context->isFinish == TRUE || context->isKilled == TRUE) {
            LOS_MuxPost(g_iperfMutex);
            break;
        }
        if (++count >= (context->param.interval * 5)) { /* 5: schedule every 200ms */
            count = 0;
            IperfReporterProcess(context, &passed, &lastTotal);
            if (context->isFinish == TRUE) {
                LOS_MuxPost(g_iperfMutex);
                break;
            }
        }
    }

    IPERF_PRINT(("OK\r\n"));
}

static int32_t IperfReporterGo(void)
{
    uint32_t ret;
    uint32_t taskID;
    TSK_INIT_PARAM_S appTask = {0};

    appTask.pfnTaskEntry = (TSK_ENTRY_FUNC)IperfReporterEntry;
    appTask.uwStackSize  = IPERF_REPORTER_STACK_SIZE;
    appTask.pcName = IPERF_REPORTER_NAME;
    appTask.usTaskPrio = IPERF_REPORTER_PRIORITY;
    appTask.uwResved   = LOS_TASK_STATUS_DETACHED;

    ret = LOS_TaskCreate(&taskID, &appTask);
    if (ret != 0) {
        IPERF_PRINT(("create reporter task failed %u\r\n", ret));
        return IPERF_FAIL;
    }
    return IPERF_OK;
}

static void IperfProcessUdpServerHdr(IperfContext *context, uint32_t len)
{
    if (len > sizeof(IperfUdpHdr) + sizeof(IperfUdpServerHdr)) {
        IperfUdpServerHdr *hdr = (IperfUdpServerHdr *)(context->param.buffer + sizeof(IperfUdpHdr));
        float end;
        uint64_t totalLen;

        end = ntohl(hdr->endSec);
        end += ntohl(hdr->endUsec) / (float)IPERF_MILLION;
        totalLen = (((uint64_t)ntohl(hdr->totalLenH)) << 32) + ntohl(hdr->totalLenL);  /* 32: bits of int32_t */

        PrintBandwidth(totalLen, 0, end);
    }
}

static void IperfSendUdpFin(IperfContext *context)
{
    int32_t ret;
    int32_t tries = 0;
    fd_set rdSet;
    struct timeval tmo;
    BOOL sndErr = FALSE;
    BOOL rcvErr = FALSE;

    while (tries++ < IPERF_UDP_FIN_MAX_RETRIES) {
        ret = send(context->trafficSock, context->param.buffer, context->param.bufLen, 0);
        if (ret < 0) {
            sndErr = TRUE;
            break;
        }

        FD_ZERO(&rdSet);
        FD_SET(context->trafficSock, &rdSet);
        tmo.tv_sec  = 0;
        tmo.tv_usec = IPERF_UDP_FIN_TMO;

        ret = select(context->trafficSock + 1, &rdSet, NULL, NULL, &tmo);
        if (ret < 0) {
            break;
        } else if (ret == 0) {
            continue;
        } else {
            ret = recv(context->trafficSock, context->param.buffer, context->param.bufLen, 0);
            if (ret < 0) {
                rcvErr = TRUE;
                break;
            }
            IperfProcessUdpServerHdr(context, (uint32_t)ret);
            return;
        }
    }

    if (((sndErr == TRUE) || (rcvErr == TRUE)) && (context->isKilled == FALSE)) {
        IPERF_PRINT(("send UDP FIN: %s error %d\r\n", ((sndErr == TRUE) ? "send" : "recv"), errno));
    }

    IPERF_PRINT(("WARNING: did not receive ack of last datagram after %d tries.\r\n", tries - 1));
}

static void IperfSendUdpFinAck(IperfContext *context)
{
    int32_t ret;
    int32_t tries = 0;
    fd_set rdSet;
    struct timeval tmo;
    IperfUdpServerHdr *hdr = NULL;
    BOOL sndErr = FALSE;
    BOOL rcvErr = FALSE;
    uint32_t len = context->param.bufLen;
    float duration = TimeSec(context->end, context->start);

    while (tries++ < IPERF_UDP_FIN_MAX_RETRIES) {
        if (len >= (sizeof(IperfUdpHdr) + sizeof(IperfUdpServerHdr))) {
            hdr = (IperfUdpServerHdr *)(context->param.buffer + sizeof(IperfUdpHdr));
            (void)memset_s(hdr, sizeof(IperfUdpServerHdr), 0, sizeof(IperfUdpServerHdr));
            hdr->flags      = htonl(IPERF_HEADER_VERSION1);
            hdr->totalLenH  = htonl((long)(context->param.total >> 32)); /* 32: bits of int32_t */
            hdr->totalLenL  = htonl((long)(context->param.total & 0xFFFFFFFF));
            hdr->endSec     = htonl((long)duration);
            hdr->endUsec    = htonl((long)((duration - (long)duration) * IPERF_MILLION));
        }

        /* already connected, no need to use sendto */
        if (send(context->trafficSock, context->param.buffer, context->param.bufLen, 0) < 0) {
            sndErr = TRUE;
            break;
        }

        FD_ZERO(&rdSet);
        FD_SET(context->trafficSock, &rdSet);
        tmo.tv_sec  = IPERF_UDP_FINACK_TMO;
        tmo.tv_usec = 0;

        ret = select(context->trafficSock + 1, &rdSet, NULL, NULL, &tmo);
        if (ret < 0) {
            break;
        } else if (ret == 0) {
            return;
        } else {
            ret = recv(context->trafficSock, context->param.buffer, context->param.bufLen, 0);
            if (ret <= 0) {
                rcvErr = TRUE;
                return;
            } else {
                len = (uint32_t)ret;
            }
        }
    }

    if (((sndErr == TRUE) || (rcvErr == TRUE)) && (context->isKilled == FALSE)) {
        IPERF_PRINT(("send UDP FINACK: %s error %d\r\n", ((sndErr == TRUE) ? "send" : "recv"), errno));
    }

    IPERF_PRINT(("WARNING: ack of last datagram failed after %d tries.\r\n", tries - 1));
}

static void IperfGenerateUdpClientHdr(IperfContext *context)
{
    if (context->param.bufLen >= sizeof(IperfUdpHdr) + sizeof(IperfUdpClientHdr)) {
        IperfUdpClientHdr *hdr = (IperfUdpClientHdr *)(context->param.buffer + sizeof(IperfUdpHdr));

        hdr->flags = 0; /* only support normal mode */
        hdr->numThreads = 1; /* only support one client (-P not supported) */
        hdr->port =    htons(context->param.port);
        hdr->bufLen =  htonl(context->param.bufLen);
        hdr->winBand = htonl((long)context->param.bandwidth);
        if ((context->param.amount & 0x80000000) != 0) {
            hdr->amount =  htonl(-(long)context->param.amount);
        } else {
            hdr->amount =  htonl((long)context->param.amount);
            hdr->amount &= htonl(0x7FFFFFFF);
        }
    }
}

/* set server socket option */
static int32_t IperfServerSetOption(IperfContext *context)
{
    struct timeval rcvTmo;
    rcvTmo.tv_sec = IPERF_DEFAULT_RX_TIMEOUT;
    rcvTmo.tv_usec = 0;
    if (setsockopt(context->trafficSock, SOL_SOCKET, SO_RCVTIMEO, &rcvTmo, sizeof(rcvTmo)) < 0) {
        IPERF_PRINT(("set rcvtimeo failed %d\r\n", errno));
        return IPERF_FAIL;
    }

    int32_t tos = context->param.tos;
    if (setsockopt(context->trafficSock, SOL_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
        IPERF_PRINT(("set reuse failed %d\r\n", errno));
        return IPERF_FAIL;
    }

    return IPERF_OK;
}

static int32_t IperfServerFirstDataProcess(IperfContext *context,
                                           const struct sockaddr *from,
                                           socklen_t len)
{
    /* start reporter when first packet received */
    if (IperfReporterGo() < 0) {
        return IPERF_FAIL;
    }

    gettimeofday(&context->start, NULL);

    if (IPERF_IS_UDP(context->param.mask) && (connect(context->trafficSock, from, len) < 0)) {
        IPERF_PRINT(("connect failed %d\r\n", errno));
        return IPERF_FAIL;
    }

    return IPERF_OK;
}

/* iperf server testing phase1:
 * 1, create socket based on domain and type;
 * 2, set reuseaddr option to makesure bind ok;
 * 3, bind to local port (5001 or -p option);
 * 4, listen and set backlog to 1 to allow only one TCP connection.
 * 5, accept the new connection in TCP testing;
 * 6, close TCP listen socket to prohibit new TCP connection.
 * 7, set socket option, include ToS, RCVTIMEO;
 *
 * return -1 if failed or socket fd if succeed.
 */
static int32_t IperfServerPhase1(IperfContext *context, int32_t domain, socklen_t addrLen,
                                 struct sockaddr *local, struct sockaddr *remote)
{
    int32_t sock;
    BOOL isUdp = IPERF_IS_UDP(context->param.mask);

    sock = socket(domain, (isUdp ? SOCK_DGRAM : SOCK_STREAM), 0);
    if (sock < 0) {
        IPERF_PRINT(("socket failed %d\r\n", errno));
        return IPERF_FAIL;
    }

    int32_t reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        IPERF_PRINT(("set reuse failed %d\r\n", errno));
        goto FAILED;
    }

    if (bind(sock, local, addrLen) < 0) {
        IPERF_PRINT(("bind failed %d\r\n", errno));
        goto FAILED;
    }

    if (isUdp == FALSE) {
        /* backlog 1 means only accept one tcp connection */
        if (listen(sock, 1) < 0) {
            IPERF_PRINT(("listen failed %d\r\n", errno));
            goto FAILED;
        }

        context->listenSock = sock;
        /* block to wait for new connection */
        int32_t ret = accept(sock, remote, &addrLen);
        if (ret < 0) {
            if (context->isKilled == FALSE) {
                IPERF_PRINT(("accept failed %d\r\n", errno));
            }
            goto FAILED;
        } else {
            closesocket(sock);
            context->listenSock = IPERF_INVALID_SOCKET;
            sock = ret;
        }
    }

    context->trafficSock = sock;
    if (IperfServerSetOption(context) < 0) {
        goto FAILED;
    }
    return sock;

FAILED:
    closesocket(sock);
    context->trafficSock = IPERF_INVALID_SOCKET;
    context->listenSock = IPERF_INVALID_SOCKET;
    return IPERF_FAIL;
}

/* iperf client testing phase2:
 * 1, recv data from iperf client;
 * 2, start reporter task when the first data received;
 * 3, stop testing if recv failed;
 * 4, send FINACK in UDP testing;
 * 5, feed watchdog for some platform;
 */
static void IperfServerPhase2(IperfContext *context)
{
    int32_t recvLen;
    struct sockaddr peer;
    struct sockaddr *from = &peer;
    socklen_t slen = sizeof(struct sockaddr);
    socklen_t *fromLen = &slen;
    BOOL firstData = FALSE;
    BOOL isUdp = IPERF_IS_UDP(context->param.mask);
    IperfUdpHdr *udpHdr = (IperfUdpHdr *)context->param.buffer;

    while ((context->isFinish == FALSE) && (context->isKilled == FALSE)) {
        recvLen = recvfrom(context->trafficSock, context->param.buffer,
                           context->param.bufLen, 0, from, fromLen);
        if (recvLen < 0) {
            if ((firstData == FALSE) && (errno == EAGAIN)) {
                continue;
            }
            IPERF_PRINT(("recv failed %d\r\n", errno));
            break;
        } else if (recvLen == 0) { /* tcp connection closed by peer side */
            context->isFinish = TRUE;
            break;
        } else {
            if (firstData == FALSE) {
                firstData = TRUE;
                IperfServerFirstDataProcess(context, from, *fromLen);
                from = NULL;
                fromLen = NULL;
            }

            context->param.total += (uint32_t)recvLen;
            if ((isUdp == TRUE) && ((int32_t)ntohl(udpHdr->id) < 0)) {
                context->isFinish = TRUE;
                break;
            }
#ifdef IPERF_FEED_WDG
            IperfFeedWdg();
#endif /* IPERF_FEED_WDG */
        }
    }

    gettimeofday(&context->end, NULL);
}

/* iperf server testing phase3:
 * 1, output testing result, include time, amount and average speed;
 * 2, send UDP FINACK to client;
 * 3, close traffic socket;
 */
static void IperfServerPhase3(IperfContext *context)
{
    if (context->start.tv_sec != 0) { /* already connected */
        float duration = TimeSec(context->end, context->start);
        IperfDelay(100000); /* 100000: delay 100ms to make reporter print first */
        PrintBandwidth(context->param.total, 0, duration);
        if (IPERF_IS_UDP(context->param.mask) && (context->isFinish == TRUE)) {
            IperfSendUdpFinAck(context);
        }
    }

    closesocket(context->trafficSock);
    context->trafficSock = IPERF_INVALID_SOCKET;
    context->isFinish = TRUE;
}

static void IperfServerGo(IperfContext *context)
{
    int32_t domain, sock;
    struct sockaddr_in local, remote;
    struct sockaddr_in6 local6, remote6;
    struct sockaddr *localAddr = NULL;
    struct sockaddr *remoteAddr = NULL;
    socklen_t addrLen;

    if (IPERF_IS_IPV6(context->param.mask)) {
        (void)memset_s(&local6, sizeof(local6), 0, sizeof(local6));
        domain = AF_INET6;
        ADDR6_INIT(domain, htons(context->param.port), context->param.u1.srcIP6, local6);
        localAddr = (struct sockaddr *)&local6;
        remoteAddr = (struct sockaddr *)&remote6;
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        domain = AF_INET;
        (void)memset_s(&local, sizeof(local), 0, sizeof(local));
        ADDR4_INIT(domain, htons(context->param.port), context->param.u1.srcIP4, local);
        localAddr = (struct sockaddr *)&local;
        remoteAddr = (struct sockaddr *)&remote;
        addrLen = sizeof(struct sockaddr_in);
    }

    sock = IperfServerPhase1(context, domain, addrLen, localAddr, remoteAddr);
    if (sock < 0) {
        return;
    }

    IperfServerPhase2(context);

    IperfServerPhase3(context);
}


/* iperf client testing phase1:
 * 1, create socket based on domain and type;
 * 2, set reuseaddr option to makesure bind ok;
 * 3, bind to local address (-B option);
 * 4, set socket option, include ToS (-S option);
 * 5, connect to peer address (-c option);
 * 6, start reporter task;
 *
 * return -1 if failed or socket fd if succeed.
 */
static int32_t IperfClientPhase1(IperfContext *context, int32_t domain, socklen_t addrLen,
                                 struct sockaddr *local, struct sockaddr *remote)
{
    int32_t sock;
    BOOL isUdp = IPERF_IS_UDP(context->param.mask);

    sock = socket(domain, (isUdp ? SOCK_DGRAM : SOCK_STREAM), 0);
    if (sock < 0) {
        IPERF_PRINT(("socket failed %d\r\n", errno));
        return IPERF_FAIL;
    }

    int32_t reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        IPERF_PRINT(("set reuseaddr failed %d\r\n", errno));
        goto FAILED;
    }

    if (bind(sock, local, addrLen) < 0) {
        IPERF_PRINT(("bind failed %d\r\n", errno));
        goto FAILED;
    }

    int32_t tos = context->param.tos;
    if (setsockopt(sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
        IPERF_PRINT(("set tos failed %d\r\n", errno));
        goto FAILED;
    }

    context->trafficSock = sock; /* Must set traffic socket before connect */
    if (connect(sock, remote, addrLen) < 0) {
        if (context->isKilled == FALSE) {
            IPERF_PRINT(("connect failed %d\r\n", errno));
        }
        goto FAILED;
    }

    if (IperfReporterGo() < 0) {
        IPERF_PRINT(("start reporter failed %d\r\n"));
        goto FAILED;
    }

    return sock;

FAILED:
    closesocket(sock);
    if (context->trafficSock > 0) {
        context->trafficSock = IPERF_INVALID_SOCKET;
    }
    return IPERF_FAIL;
}

static void IperfClientUdpPre(IperfContext *context, int32_t *delayTarget)
{
    IperfGenerateUdpClientHdr(context); /* construct udp client header */
    *delayTarget = (int)((uint64_t)context->param.bufLen *
                         ((IPERF_BITS_PER_BYTE * (float)IPERF_MILLION) / context->param.bandwidth));
    if ((*delayTarget < 0) || (*delayTarget > IPERF_MILLION)) {
        *delayTarget = IPERF_MILLION;
    }
}

/* iperf client testing phase2:
 * 1, construct client header in UDP testing, include packet ID and time;
 * 2, record testing start time;
 * 3, send data to iperf server;
 * 4, stop testing if the total amout reached (-n option) or send failed;
 * 5, adjust sending rate in UDP testing;
 * 6, feed watchdog for some platform;
 * 7, record testing end time;
 */
static void IperfClientPhase2(IperfContext *context)
{
    struct timeval lastPktTime, packetTime;
    IperfUdpHdr *udpHdr = (IperfUdpHdr*)context->param.buffer;
    int32_t delay = 0;
    int32_t adjust = 0;
    int32_t delayTarget = 0;
    int32_t sendLen;
    BOOL isUdp = IPERF_IS_UDP(context->param.mask);

    if (isUdp) {
        gettimeofday(&lastPktTime, NULL);
        IperfClientUdpPre(context, &delayTarget);
    }

    gettimeofday(&context->start, NULL);
    while ((context->isKilled == FALSE) && (context->isFinish == FALSE)) {
        if (isUdp) {
            gettimeofday(&packetTime, NULL);
            udpHdr->id   = htonl(context->udpPktID++);
            udpHdr->sec  = htonl(packetTime.tv_sec);
            udpHdr->usec = htonl(packetTime.tv_usec);

            adjust = delayTarget + TimeUsec(lastPktTime, packetTime);
            lastPktTime.tv_sec  = packetTime.tv_sec;
            lastPktTime.tv_usec = packetTime.tv_usec;

            if ((adjust > 0) || (delay > 0)) {
                delay += adjust;
            }

            if (delay > (int32_t)IPERF_UDP_DELAY) {
                IperfDelay(delay);
            }
        }

        sendLen = send(context->trafficSock, context->param.buffer, context->param.bufLen, 0); /* 0: no flag */
        if (sendLen < 0) {
            if (errno == ENOMEM) {
                IperfDelay(0); /* 0: delay one tick */
                continue;
            } else {
                IPERF_PRINT(("send failed %d\r\n", errno));
                break;
            }
        }

        context->param.total += (uint32_t)sendLen;
        if ((context->param.amount != 0) && (context->param.total >= context->param.amount)) {
            break;
        }
#ifdef IPERF_FEED_WDG
        IperfFeedWdg();
#endif /* IPERF_FEED_WDG */
    }
    gettimeofday(&context->end, NULL);
}

/* iperf client testing phase3:
 * 1, send FIN packet in UDP testing;
 * 2, output testing result, include time, amount and average speed;
 * 3, close traffic socket;
 */
static void IperfClientPhase3(IperfContext *context)
{
    BOOL isUdp = IPERF_IS_UDP(context->param.mask);
    struct timeval packetTime;
    IperfUdpHdr *udpHdr = (IperfUdpHdr*)context->param.buffer;

    context->isFinish = TRUE;

    if (isUdp) {
        gettimeofday(&packetTime, NULL);
        udpHdr->id = htonl(-(context->udpPktID)); /* negative value */
        udpHdr->sec = htonl(packetTime.tv_sec);
        udpHdr->usec = htonl(packetTime.tv_usec);
        IperfSendUdpFin(context);
    } else {
        float duration = TimeSec(context->end, context->start);
        PrintBandwidth(context->param.total, 0, duration);
    }

    closesocket(context->trafficSock);
    context->trafficSock = IPERF_INVALID_SOCKET;
}

static void IperfClientGo(IperfContext *context)
{
    int32_t domain, sock;
    struct sockaddr_in local, remote;
    struct sockaddr_in6 local6, remote6;
    struct sockaddr *localAddr = NULL;
    struct sockaddr *remoteAddr = NULL;
    socklen_t addrLen;

    if (IPERF_IS_IPV6(context->param.mask)) {
        domain = AF_INET6;
        (void)memset_s(&local6, sizeof(local6), 0, sizeof(local6));
        (void)memset_s(&remote6, sizeof(remote6), 0, sizeof(remote6));
        ADDR6_INIT(domain, 0, context->param.u1.srcIP6, local6);
        ADDR6_INIT(domain, htons(context->param.port), context->param.u2.dstIP6, remote6);
        localAddr = (struct sockaddr *)&local6;
        remoteAddr = (struct sockaddr *)&remote6;
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        domain = AF_INET;
        (void)memset_s(&local, sizeof(local), 0, sizeof(local));
        (void)memset_s(&remote, sizeof(remote), 0, sizeof(remote));
        ADDR4_INIT(domain, 0, context->param.u1.srcIP4, local);
        ADDR4_INIT(domain, htons(context->param.port), context->param.u2.dstIP4, remote);
        localAddr = (struct sockaddr *)&local;
        remoteAddr = (struct sockaddr *)&remote;
        addrLen = sizeof(struct sockaddr_in);
    }

    sock = IperfClientPhase1(context, domain, addrLen, localAddr, remoteAddr);
    if (sock < 0) {
        return;
    }

    struct timeval sndTmo;
    sndTmo.tv_sec = IPERF_DEFAULT_TX_TIMEOUT;
    sndTmo.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sndTmo, sizeof(sndTmo));

    IperfClientPhase2(context);

    IperfClientPhase3(context);
}


static void IperfTrafficEntry(uint32_t p0)
{
    IperfContext *context = (IperfContext *)p0;

    if (LOS_MuxCreate(&g_iperfMutex) != LOS_OK) {
        return;
    }

    context->param.buffer = (uint8_t *)malloc(context->param.bufLen);
    if (context->param.buffer == NULL) {
        IPERF_PRINT(("Error: no free memory\r\n"));
        goto DONE;
    }

    for (uint32_t i = 0; i < context->param.bufLen; i++) {
        context->param.buffer[i] = (i % 10) + '0'; /* 10: iperf data is '0' to '9' */
    }

    context->isRunning = TRUE;

    (void)LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER);
    g_iperfContext = context;
    (void)LOS_MuxPost(g_iperfMutex);

    if (IPERF_IS_SERVER(context->param.mask)) {
        IperfServerGo(context);
    } else {
        IperfClientGo(context);
    }

DONE:
    LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER);
    if (context->param.buffer != NULL) {
        free(context->param.buffer);
        context->param.buffer = NULL;
    }

    free(context);
    g_iperfContext = NULL;
    LOS_MuxPost(g_iperfMutex);

    uint32_t mtx = g_iperfMutex;
    g_iperfMutex = 0xFFFFFFFF; /* 0xFFFFFFFF: invalid handle */
    LOS_MuxDelete(mtx);
}

static int32_t IperfStart(IperfContext *context)
{
    uint32_t ret;
    uint32_t taskID;
    TSK_INIT_PARAM_S appTask = {0};

    appTask.pfnTaskEntry = (TSK_ENTRY_FUNC)IperfTrafficEntry;
    appTask.uwStackSize  = IPERF_TRAFFIC_STACK_SIZE;
    appTask.pcName = IPERF_TRAFFIC_NAME;
    if (IPERF_IS_SERVER(context->param.mask)) {
        appTask.usTaskPrio = IPERF_TRAFFIC_SERVER_PRIORITY;
    } else {
        appTask.usTaskPrio = IPERF_TRAFFIC_CLIENT_PRIORITY;
    }
    appTask.uwResved = LOS_TASK_STATUS_DETACHED;
    appTask.auwArgs[0] = (uint32_t)context;

    ret = LOS_TaskCreate(&taskID, &appTask);
    if (ret != 0) {
        IPERF_PRINT(("create traffic task failed %u\r\n", ret));
        free(context);
        return IPERF_FAIL;
    }
    return IPERF_OK;
}

static int32_t IperfStop(void)
{
    if (LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return IPERF_FAIL;
    }
    if (IsIperfRunning()) {
        g_iperfContext->isKilled = TRUE;
    } else {
        (void)LOS_MuxPost(g_iperfMutex);
        return IPERF_FAIL;
    }
    (void)LOS_MuxPost(g_iperfMutex);

    LOS_TaskDelay(LOSCFG_BASE_CORE_TICK_PER_SECOND); /* 1000ms: makesure atleast three UDP FIN send out */

    if (LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return IPERF_OK; /* iperf already stopped */
    }
    IperfContext *context = g_iperfContext;
    if ((context != NULL) && (context->listenSock != IPERF_INVALID_SOCKET)) {
        closesocket(context->listenSock);
        context->listenSock = IPERF_INVALID_SOCKET;
    }
    if ((context != NULL) && (context->trafficSock != IPERF_INVALID_SOCKET)) {
        closesocket(context->trafficSock);
        context->trafficSock = IPERF_INVALID_SOCKET;
    }

    (void)LOS_MuxPost(g_iperfMutex);
    return IPERF_OK;
}

static BOOL IperfByteAtoi(const char *str, uint64_t *out)
{
    int64_t num;
    uint8_t suffix;
    char arr[32]; /* 32: max string len */

    if (str == NULL || (strlen(str) <= 0) || (strlen(str) >= sizeof(arr) - 1)) {
        return FALSE;
    }

    if (strcpy_s(arr, sizeof(arr), str) != EOK) {
        return FALSE;
    }

    suffix = arr[strlen(arr) - 1];
    if ((suffix == 'G') || (suffix == 'M') || (suffix == 'K') ||
        (suffix == 'g') || (suffix == 'm') || (suffix == 'k')) {
        arr[strlen(arr) - 1] = 0;
    } else {
        suffix = '\n';
    }

    num = atoi(arr);
    if (num <= 0) {
        return FALSE;
    }

    switch (suffix) {
        case 'G':
            num *= IPERF_GIGA;
            break;
        case 'M':
            num *= IPERF_MEGA;
            break;
        case 'K':
            num *= IPERF_KILO;
            break;
        case 'g':
            num *= IPERF_BILLION;
            break;
        case 'm':
            num *= IPERF_MILLION;
            break;
        case 'k':
            num *= IPERF_THOUSAND;
            break;
        case '\n':
            num = num;
            break;
        default:
            return FALSE;
    }
    *out = (uint64_t)num;
    return TRUE;
}

#ifdef IPERF_DEBUG
static void IperfInfoShow(IperfContext *context)
{
#define IPERF_MAX_ADDRLEN 64
    char sIP[IPERF_MAX_ADDRLEN], dIP[IPERF_MAX_ADDRLEN];
    int32_t domain = ((IPERF_IS_IPV6(context->param.mask)) ? AF_INET6 : AF_INET);

    IPERF_PRINT(("Iperf info: %s-%s-%s\r\n",
           ((IPERF_IS_UDP(context->param.mask)) ? "UDP" : "TCP"),
           ((IPERF_IS_CLIENT(context->param.mask)) ? "Client" : "Server"),
           ((domain == AF_INET) ? "IPV4" : "IPV6")));
    inet_ntop(domain, &context->param.u1.srcIP6, sIP, IPERF_MAX_ADDRLEN);
    inet_ntop(domain, &context->param.u2.dstIP6, dIP, IPERF_MAX_ADDRLEN);
    IPERF_PRINT(("sIP: %s, dIP: %s, port: %d\r\n", sIP, dIP, context->param.port));

    IPERF_PRINT(("Interval: %usec, time: %usec, amount: %llu, bandwidth: %llu, bufLen: %u\r\n",
           context->param.interval, context->param.time, context->param.amount,
           context->param.bandwidth, context->param.bufLen));
#undef IPERF_MAX_ADDRLEN
}
#endif

static BOOL IperfOptionConv(int32_t index, int32_t argc, const char **argv, uint32_t *para)
{
    if ((index + 1) == argc) {
        IPERF_PRINT(("parameter %s is error\r\n", argv[index]));
        return FALSE;
    }
    *para = atoi(argv[++index]);
    return TRUE;
}

/* without value */
static void ParseParam1(IperfContext *context, int32_t argc, const char **argv)
{
    for (int32_t i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-V")) { /* Must be the first option */
            context->param.mask |= IPERF_MASK_IPV6;
        }

        if (!strcmp(argv[i], "-u")) {
            context->param.mask |= IPERF_MASK_UDP;
        }

        if (!strcmp(argv[i], "-s")) {
            context->param.mask |= IPERF_MASK_SERVER;
        }
    }
}

/* with value, but not IP address */
static int32_t ParseParam2(IperfContext *context, int32_t argc, const char **argv)
{
    for (int32_t i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-i")) {
            if (IperfOptionConv(i++, argc, argv, &context->param.interval) == FALSE) {
                return IPERF_FAIL;
            }
        }

        if (!strcmp(argv[i], "-p")) {
            uint32_t port;
            if (IperfOptionConv(i++, argc, argv, &port) == FALSE) {
                return IPERF_FAIL;
            }
            context->param.port = port;
        }

        if (!strcmp(argv[i], "-t")) {
            uint32_t time;
            if (IperfOptionConv(i++, argc, argv, &time) == FALSE) {
                return IPERF_FAIL;
            }
            if (time > IPERF_DEFAULT_MAX_TIME) {
                time = IPERF_DEFAULT_MAX_TIME;
            }
            context->param.time = time;
        }

        if (!strcmp(argv[i], "-S")) {
            uint32_t tos;
            if (IperfOptionConv(i++, argc, argv, &tos) == FALSE) {
                return IPERF_FAIL;
            }
            context->param.tos = tos;
        }
    }
    return IPERF_OK;
}

/* with IP address */
static int32_t ParseParam3(IperfContext *context, int32_t argc, const char **argv)
{
    int32_t domain = IPERF_IS_IPV6(context->param.mask) ? AF_INET6 : AF_INET;

    for (int32_t i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-B")) {
            if ((i + 1) == argc) {
                IPERF_PRINT(("srcIP is needed\r\n"));
                return IPERF_FAIL;
            }
            if (inet_pton(domain, argv[++i], (void *)&context->param.u1.srcIP6) <= 0) {
                IPERF_PRINT(("srcIP is invalid\r\n"));
                return IPERF_FAIL;
            }
        }

        if (!strcmp(argv[i], "-c")) {
            context->param.mask |= IPERF_MASK_CLIENT;
            if ((i + 1) == argc) {
                IPERF_PRINT(("dstIP is needed\r\n"));
                return IPERF_FAIL;
            }
            if (inet_pton(domain, argv[++i], (void *)&context->param.u2.dstIP6) <= 0) {
                IPERF_PRINT(("dstIP is invalid\r\n"));
                return IPERF_FAIL;
            }
        }
    }

    return IPERF_OK;
}

/* -b (bandwidth) */
static int32_t ParseParamLowerB(IperfContext *context, int32_t argc, const char **argv)
{
    int32_t i;
    BOOL found = FALSE;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-b")) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        return IPERF_OK;
    }

    BOOL isUdpClient = IPERF_IS_CLIENT(context->param.mask) && IPERF_IS_UDP(context->param.mask);
    if (isUdpClient == FALSE) {
        IPERF_PRINT(("-b only allowed for UDP Client\r\n"));
        return IPERF_FAIL;
    }
    if ((i + 1) == argc) {
        IPERF_PRINT(("bandwidth is needed\r\n"));
        return IPERF_FAIL;
    }
    if (IperfByteAtoi(argv[++i], &context->param.bandwidth) == FALSE) {
        IPERF_PRINT(("bandwidth is invalid\r\n"));
        return IPERF_FAIL;
    }

    return IPERF_OK;
}

/* -n (amount) */
static int32_t ParseParamLowerN(IperfContext *context, int32_t argc, const char **argv)
{
    int32_t i;
    BOOL found = FALSE;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-n")) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        return IPERF_OK;
    }

    BOOL isClient = IPERF_IS_CLIENT(context->param.mask);
    if (isClient == FALSE) {
        IPERF_PRINT(("-n only allowed for Client\r\n"));
        return IPERF_FAIL;
    }
    if ((i + 1) == argc) {
        IPERF_PRINT(("amount is needed\r\n"));
        return IPERF_FAIL;
    }
    if (IperfByteAtoi(argv[++i], &context->param.amount) == FALSE) {
        IPERF_PRINT(("amount is invalid\r\n"));
        return IPERF_FAIL;
    }

    return IPERF_OK;
}

/* -l (send or receive buffer length) */
static int32_t ParseParamLowerL(IperfContext *context, int32_t argc, const char **argv)
{
    int32_t i;
    BOOL found = FALSE;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-l")) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        return IPERF_OK;
    }

    if ((i + 1) == argc) {
        IPERF_PRINT(("buffer length is needed\r\n"));
        return IPERF_FAIL;
    }
    uint64_t bufLen;
    if (IperfByteAtoi(argv[++i], &bufLen) == FALSE) {
        IPERF_PRINT(("buffer length is invalid\r\n"));
        return IPERF_FAIL;
    }
    if (bufLen > IPERF_DEFAULT_TCP_BUFLEN * 2 || /* 2: buffer length must be smaller than 16KB */
        bufLen < sizeof(IperfUdpHdr) + sizeof(IperfUdpClientHdr)) {
        IPERF_PRINT(("buffer length should be [36, 16K]\r\n"));
        return IPERF_FAIL;
    }

    context->param.bufLen = bufLen;
    return IPERF_OK;
}
static int32_t IperfParseParameter(IperfContext *context, int32_t argc, const char **argv)
{
    ParseParam1(context, argc, argv); /* -V, -u, -s */
    if (ParseParam2(context, argc, argv) < 0) { /* -i, -p, -t, -S */
        return IPERF_FAIL;
    }
    if (ParseParam3(context, argc, argv) < 0) { /* -c, -B */
        return IPERF_FAIL;
    }
    if (ParseParamLowerB(context, argc, argv) < 0) { /* -u */
        return IPERF_FAIL;
    }
    if (ParseParamLowerN(context, argc, argv) < 0) { /* -n */
        return IPERF_FAIL;
    }
    if (ParseParamLowerL(context, argc, argv) < 0) { /* -l */
        return IPERF_FAIL;
    }

    if ((context->param.time != 0) && (context->param.amount != 0)) {
        IPERF_PRINT(("Error: only need one end condition -t(time) or -n(amount)"));
        return IPERF_FAIL;
    }

    return IPERF_OK;
}

void IperfSetDefaultParameter(IperfContext *context)
{
    if (context->param.port == 0) {
        context->param.port = IPERF_DEFAULT_PORT;
    }

    if (context->param.interval == 0) {
        context->param.interval = IPERF_DEFAULT_INTERVAL;
    }

    if ((context->param.time == 0) && (context->param.amount == 0) &&
        IPERF_IS_CLIENT(context->param.mask)) {
        context->param.time = IPERF_DEFAULT_TIME;
    }

    if ((context->param.bandwidth == 0) && IPERF_IS_UDP(context->param.mask) &&
        IPERF_IS_CLIENT(context->param.mask)) {
        context->param.bandwidth = IPERF_DEFAULT_UDP_BW;
    }

    if (context->param.bufLen == 0) {
        context->param.bufLen = ((IPERF_IS_UDP(context->param.mask)) ?
                                  IPERF_DEFAULT_UDP_BUFLEN : IPERF_DEFAULT_TCP_BUFLEN);
    }
}

void IperfHelp(void)
{
    IPERF_PRINT(("Usage: iperf [-s|-c host] [options]\r\n"));
    IPERF_PRINT(("Try 'iperf --help' for more information.\r\n"));
}

static void IperfUsage(void)
{
    IPERF_PRINT(("Usage: iperf [-s|-c host] [options]\r\n"));
    IPERF_PRINT(("       iperf [-h|-help|--help]\r\n"));
    IPERF_PRINT(("Client/Server:\r\n"));
    IPERF_PRINT(("  -i, #[interval]  pause n seconds between periodic bandwidth reports(default 1 secs)\r\n"));
    IPERF_PRINT(("  -l, #[length]    length of buffer to read or write, defalut 1470(UDP)/8KB(TCP)\r\n"));
    IPERF_PRINT(("  -u,              use UDP rather than TCP\r\n"));
    IPERF_PRINT(("  -p, #[port]      set server port to listen on/connect to to n (default 5001)\r\n"));
    IPERF_PRINT(("  -V,              set the domain to IPv6(Must be 1st parameter)\r\n"));
    IPERF_PRINT(("  -S, #[tos]       set the IP ToS, 0-255, default 0\r\n"));
    IPERF_PRINT(("  -B, #<host>      bind to <host>, an unicast address, not support multicast\r\n"));
    IPERF_PRINT(("  -k               kill the iperf\r\n"));
    IPERF_PRINT(("Client specific:\r\n"));
    IPERF_PRINT(("  -c, #[host]      run in client mode, connecting to <host>\r\n"));
    IPERF_PRINT(("  -t, #[time]      time in seconds to transmit for (default 30 secs)\r\n"));
    IPERF_PRINT(("  -n, #[KM]        number of bytes to transmit (instead of -t)\r\n"));
    IPERF_PRINT(("  -b, #[KM]        for UDP, bandwidth to send in bits/sec(default 1 Mbits/sec)\r\n"));
    IPERF_PRINT(("Server specific:\r\n"));
    IPERF_PRINT(("  -s,              run in server mode\r\n"));
}

static BOOL IperfCheckSingleParam(char cmd, char* allowed)
{
    uint32_t cnt = strlen(allowed);
    for (uint32_t j = 0; j < cnt; j++) {
        if (cmd == allowed[j]) {
            return TRUE;
        }
    }

    return FALSE;
}

static int32_t IperfCheckParameter(int32_t argc, const char **argv)
{
    int32_t i;
    const char *ptr = NULL;
    char *allowed = "ilupctnbskVSB";
    BOOL valid = TRUE;

    for (i = 0; i < argc; i++) {
        ptr = argv[i];
        if (*ptr == '-') {
            ++ptr;
            if ((IperfCheckSingleParam(*ptr, allowed) == FALSE) ||
                (strlen(ptr) > 1)) { /* 1: only one alpha */
                valid = FALSE;
                break;
            }
        }
    }
    if (valid == FALSE) {
        IPERF_PRINT(("option %s is invalid\r\n", argv[i]));
        IperfHelp();
        return IPERF_FAIL;
    }
    return IPERF_OK;
}

uint32_t cmd_iperf(int32_t argc, const char **argv)
{
    if (argc < 1) {
        IperfHelp();
        return (uint32_t)IPERF_FAIL;
    }

    if ((!strcmp(argv[0], "--help")) || (!strcmp(argv[0], "-help")) || (!strcmp(argv[0], "-h"))) {
        IperfUsage();
        return IPERF_OK;
    }

    /* check if any option unsupported */
    if (IperfCheckParameter(argc, argv) < 0) {
        return IPERF_FAIL;
    }

    if (!strcmp(argv[0], "-k")) {
        if (IperfStop() < 0) {
            IPERF_PRINT(("No iperf is running\r\n"));
            return (uint32_t)IPERF_FAIL;
        } else {
            return IPERF_OK;
        }
    } else {
        if (LOS_MuxPend(g_iperfMutex, LOS_WAIT_FOREVER) == LOS_OK) {
            /* mux available means iperf running. */
            IPERF_PRINT(("Iperf is running\r\n"));
            (void)LOS_MuxPost(g_iperfMutex);
            return (uint32_t)IPERF_FAIL;
        }
    }

    IperfContext *context = malloc(sizeof(IperfContext));
    if (context == NULL) {
        IPERF_PRINT(("no free memory\r\n"));
        return (uint32_t)IPERF_FAIL;
    }
    (void)memset_s(context, sizeof(IperfContext), 0, sizeof(IperfContext));
    context->trafficSock = IPERF_INVALID_SOCKET;
    context->listenSock = IPERF_INVALID_SOCKET;

    if (IperfParseParameter(context, argc, argv) < 0) {
        goto FAILED;
    }

    IperfSetDefaultParameter(context);
#ifdef IPERF_DEBUG
    IperfInfoShow(context);
#endif

    return (uint32_t)IperfStart(context);

FAILED:
    if (context != NULL) {
        free(context);
    }
    return (uint32_t)IPERF_FAIL;
}

