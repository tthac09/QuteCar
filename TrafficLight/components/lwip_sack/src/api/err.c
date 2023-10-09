/**
 * @file
 * Error Management module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/err.h"
#include "lwip/def.h"
#include "lwip/sys.h"

#include "lwip/errno.h"

#ifdef LWIP_DEBUG

static const char *err_strerr[] = {
  "Ok.",                        /* ERR_OK          0  */
  "Out of memory error.",       /* ERR_MEM        -1  */
  "Buffer error.",              /* ERR_BUF        -2  */
  "Timeout.",                   /* ERR_TIMEOUT    -3  */
  "Routing problem.",           /* ERR_RTE        -4  */
  "Operation in progress.",     /* ERR_INPROGRESS -5  */
  "Illegal value.",             /* ERR_VAL        -6  */
  "Operation would block.",     /* ERR_WOULDBLOCK -7  */
  "Address in use.",            /* ERR_USE        -8  */
  "Already connecting.",        /* ERR_ALREADY    -9  */
  "Already connected.",         /* ERR_ISCONN     -10 */
  "Not connected.",             /* ERR_CONN       -11 */
  "Low-level netif error.",     /* ERR_IF         -12 */
  "Message too long.",          /* ERR_MSGSIZE    -13 */
  "No such device.",            /* ERR_NODEV      -14 */
  "No such device or address.", /* ERR_NODEVADDR   -15 */
  "socket is not connection-mode & no peer address is set.", /* ERR_NODEST  -16 */
  "Network is down.",          /* ERR_NETDOWN    -17 */
  "Address family not supported by protocol.",     /* ERR_AFNOSUPP    -18 */
  "Cannot assign requested address",              /* ERR_NOADDR     -19    */
  "Operation not supported on transport endpoint", /* ERR_OPNOTSUPP  -20    */
  "No route to the network",    /* ERR_NETUNREACH -21    */
  "Connection request timedout", /* ERR_CONNECTIMEOUT -22 */
  "PIPE Error.",                /* ERR_PIPE       -23    */
  "AF not supported",           /* ERR_AFNOSUPPORT -24   */
  "Protocol not available",     /* ERR_NOPROTOOPT -25    */
  "Reserved for future use",    /* ERR_RESERVE1   -26    */
  "Reserved for future use",    /* ERR_RESERVE2   -27    */
  "Reserved for future use",    /* ERR_RESERVE3   -28    */
  "Reserved for future use",    /* ERR_RESERVE4   -29    */
  "Reserved for future use",    /* ERR_RESERVE5   -30    */
  "Reserved for future use",    /* ERR_RESERVE6   -31    */
  "Reserved for future use",    /* ERR_RESERVE7   -32    */
  "Connection aborted.",        /* ERR_ABRT       -33 */
  "Connection reset.",          /* ERR_RST        -34 */
  "Connection closed.",         /* ERR_CLSD       -35 */
  "Illegal argument.",          /* ERR_ARG        -36 */
  "Connection refused.",        /* ERR_CONNREFUSED       -37 */
  "Invalid error code"          /* ERR_MAX_VAL    -38    */
};

/**
 * Convert an lwip internal error to a string representation.
 *
 * @param err an lwip internal err_t
 * @return a string representation for err
 */
const char *
lwip_strerr(err_t err)
{
  if ((err > 0) || (-err >= (err_t)LWIP_ARRAYSIZE(err_strerr))) {
    return "Unknown error.";
  }
  return err_strerr[-err];
}

#endif /* LWIP_DEBUG */
